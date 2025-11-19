// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include "cpptask/details/promise_type.h"
#include "cpptask/details/shared_task.h"
#include "cpptask/details/task_awaiter.h"
#include "cpptask/details/async_void_promise_type.h"
#include <coroutine>

namespace cpptask
{
	template<class T = void>
	class [[nodiscard]] task
	{
		template<class U>
		friend class task;

	public:
		using promise_type = ::cpptask::details::promise_type<T, task<T>>;
		using value_t = T;

	private:
		std::shared_ptr<details::shared_task<T>> _task;

	public:
		task() = default;
		task(const task&) = default;
		
		task(std::shared_ptr<details::shared_task<T>> task)
			: _task(std::move(task))
		{
		}

		bool is_valid() const noexcept
		{
			return _task;
		}

		details::task_awaiter<T> get_awaiter() const noexcept
		{
			return details::task_awaiter<T>(_task);
		}

		task_status get_status() const noexcept
		{
			return _task->get_status();
		}

		std::exception_ptr get_exception() const noexcept
		{
			return _task->get_exception();
		}

		T get_result() const
		{
			return _task->get_result();
		}

		task& operator =(const task&) = default;
		task& operator =(task&&) = default;

		auto operator <=>(const task&) const = default;
		bool operator ==(const task&) const = default;

	public:
		template<class TBody>
		static auto run(TBody&& body, std::stop_token s_token = {}) -> task<std::invoke_result_t<TBody>>
		{
			static_assert(std::is_void_v<T>, "Use task<>::run instead.");
			
			using U = std::invoke_result_t<TBody>;
			std::shared_ptr u_task = std::make_shared<details::shared_task<U>>(s_token);

			details::thread_pool::queue_user_work_item([u_task, body = std::forward<TBody>(body)]() mutable
			{
				u_task->transit_to_running();

				try
				{
					if constexpr (std::is_void_v<U>)
					{
						body();
						u_task->set_result();
					}
					else
					{
						auto r = body();
						u_task->set_result(std::move(r));
					}
				}
				catch (...)
				{
					u_task->try_set_exception(std::current_exception());
				}
			});
			return task<U>(u_task);
		}

		template<class TBody>
		static auto start_new(TBody&& body, std::stop_token s_token = {}) -> task<std::invoke_result_t<TBody>>
		{
			static_assert(std::is_void_v<T>, "Use task<>::run instead.");
			
			using U = std::invoke_result_t<TBody>;
			std::shared_ptr u_task = std::make_shared<details::shared_task<U>>(s_token);

			std::thread([u_task, body = std::forward<TBody>(body)]() mutable
			{
				u_task->transit_to_running();

				try
				{
					if constexpr (std::is_void_v<U>)
					{
						body();
						u_task->set_result();
					}
					else
					{
						auto r = body();
						u_task->set_result(std::move(r));
					}
				}
				catch (...)
				{
					u_task->try_set_exception(std::current_exception());
				}
			}).detach();
			return task<U>(u_task);
		}

		static task<> delay(std::chrono::nanoseconds delay, std::stop_token s_token = {})
		{
			static_assert(std::is_void_v<T>, "Use task<>::delay instead.");

			std::shared_ptr u_task = std::make_shared<details::shared_task<void>>(s_token);
			details::thread_pool::queue_delayed_user_work_item(delay, [u_task]() mutable
			{
				u_task->transit_to_running();
				u_task->set_result();
			});

			return task<>(std::move(u_task));
		}

		static task<> completed_task()
		{
			static_assert(std::is_void_v<T>, "Use task<>::completed_task instead.");

			static thread_local std::shared_ptr s_task = []
			{
				auto ptr = std::make_shared<details::shared_task<void>>();
				ptr->set_result();
				return ptr;
			}();

			return task<>(s_task);
		}

		template<class U> requires (!std::same_as<U, void>)
		static task<U> from_result(U in_value)
		{
			static_assert(std::is_void_v<T>, "Use task<>::from_reuslt<U> instaed.");

			auto u_task = std::make_shared<details::shared_task<U>>();
			u_task->set_result(std::move(in_value));
			return task<U>(u_task);
		}
	};
}

template<class TOwningClass, class... TArgs>
struct std::coroutine_traits<void, TOwningClass&, TArgs...>
{
	using promise_type = cpptask::details::async_void_promise_type;
};

template<class... TArgs>
struct std::coroutine_traits<void, TArgs...>
{
	using promise_type = cpptask::details::async_void_promise_type;
};