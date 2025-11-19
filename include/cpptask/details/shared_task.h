// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include "cpptask/task_status.h"
#include "cpptask/task_canceled_exception.h"
#include "cpptask/details/voidable_optional.h"
#include "cpptask/details/thread_pool.h"
#include <mutex>
#include <condition_variable>
#include <functional>
#include <exception>
#include <optional>
#include <stop_token>
#include <cassert>

namespace cpptask::details
{
	template<class T>
	class shared_task
	{
	private:
		mutable std::mutex _mutex;
		mutable std::condition_variable _future;

		voidable_optional<T> _promise;
		task_status _status = task_status::created;
		std::exception_ptr _exception_ptr;

		std::vector<std::move_only_function<void()>> _continuations;
		std::optional<std::stop_callback<std::move_only_function<void()>>> _sc;

	public:
		shared_task(std::stop_token s_token = {})
		{
			if (s_token.stop_possible())
			{
				_sc.emplace(s_token, [this]()
				{
					this->try_cancel();
				});
			}
		}

		shared_task(const shared_task&) = delete;
		~shared_task() noexcept
		{
		}

		void transit_to_running()
		{
			std::unique_lock lock(_mutex);
			assert(_status == task_status::created);
			_status = task_status::running;
		}

		task_status get_status() const noexcept
		{
			return _status;
		}

		void then(std::move_only_function<void()> continuation)
		{
			std::unique_lock lock(_mutex);
			if (is_completed())
			{
				lock.unlock();
				thread_pool::queue_user_work_item(std::move(continuation));
			}
			else
			{
				_continuations.emplace_back(std::move(continuation));
			}
		}

		std::exception_ptr get_exception() const
		{
			return _exception_ptr;
		}

		T get_result() const
		{
			wait();
			if (_exception_ptr)
			{
				std::rethrow_exception(_exception_ptr);
			}
			return _promise.get_value();
		}

		void wait() const noexcept
		{
			if (is_completed())
			{
				return;
			}

			std::unique_lock lock(_mutex);
			_future.wait(lock, [this] { return is_completed(); });
		}

		template<class... U>
		void set_result(U&&... args) requires
			(std::same_as<T, void> && sizeof...(U) == 0) ||
			(!std::same_as<T, void> && std::constructible_from<T, U...>)
		{
			std::unique_lock lock(_mutex);
			if (is_completed())
			{
				throw std::runtime_error("Task already completed.");
			}

			assert(_status == task_status::running);
			_status = task_status::ran_to_completion;
			_promise.emplace(std::forward<U>(args)...);

			auto cc = std::move(_continuations);
			_future.notify_all();
			lock.unlock();
			for (auto& c : cc)
			{
				thread_pool::queue_user_work_item(std::move(c));
			}
		}

		bool try_cancel() noexcept
		{
			std::unique_lock lock(_mutex);
			if (is_completed())
			{
				return false;
			}

			assert(_status == task_status::running || _status == task_status::created);
			_status = task_status::canceled;
			_exception_ptr = std::make_exception_ptr(task_canceled_exception());

			auto cc = std::move(_continuations);
			_future.notify_all();
			lock.unlock();
			for (auto& c : cc)
			{
				thread_pool::queue_user_work_item(std::move(c));
			}

			return true;
		}

		bool try_set_exception(std::exception_ptr except) noexcept
		{
			std::unique_lock lock(_mutex);
			if (is_completed())
			{
				return false;
			}

			try
			{
				std::rethrow_exception(except);
			}
			catch (const task_canceled_exception&)
			{
				assert(_status == task_status::running || _status == task_status::created);
				_status = task_status::canceled;
				_exception_ptr = std::current_exception();
			}
			catch (...)
			{
				assert(_status == task_status::running);
				_status = task_status::faulted;
				_exception_ptr = std::current_exception();
			}

			auto cc = std::move(_continuations);
			_future.notify_all();
			lock.unlock();
			for (auto& c : cc)
			{
				thread_pool::queue_user_work_item(std::move(c));
			}

			return true;
		}

		bool is_completed() const noexcept
		{
			auto status = get_status();
			return status == task_status::ran_to_completion || status == task_status::faulted || status == task_status::canceled;
		}
	};
}