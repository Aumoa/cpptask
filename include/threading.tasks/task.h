// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <optional>
#include <cassert>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <coroutine>
#include <stop_token>
#include <vector>
#include <concepts>
#include <type_traits>
#include <ranges>
#include <expected>

namespace threading::tasks
{
	namespace details
	{
		template<class T>
		class voidable_optional
		{
			bool _has_value;
			char* _buffer[sizeof(T)];

		public:
			constexpr voidable_optional() noexcept
				: _has_value(false)
#ifndef NDEBUG
				, _buffer{}
#endif
			{
			}

			constexpr voidable_optional(std::nullopt_t) noexcept
				: _has_value(false)
#ifndef NDEBUG
				, _buffer{}
#endif
			{
			}

			template<class U>
			constexpr voidable_optional(U&& in_value) noexcept
				: _has_value(true)
			{
				new(_buffer) T(std::forward<U>(in_value));
			}

			constexpr voidable_optional(const voidable_optional& rhs) noexcept(noexcept(new(_buffer) T(rhs.value())))
				: _has_value(rhs._has_value)
			{
				if (rhs._has_value)
				{
					new(_buffer) T(rhs.get_value());
				}
			}

			constexpr voidable_optional(voidable_optional&& rhs) noexcept
				: _has_value(rhs._has_value)
			{
				if (rhs._has_value)
				{
					new(_buffer) T(std::move(rhs.get_value()));
					rhs._has_value = false;
				}
			}

			constexpr ~voidable_optional() noexcept
			{
				reset();
			}

			constexpr voidable_optional& reset() noexcept
			{
				if (_has_value)
				{
					get_value().~T();
					_has_value = false;
				}
				return *this;
			}

			template<class U>
			constexpr voidable_optional& set_value(U&& in_value) noexcept(noexcept(new(_buffer) T(std::forward<U>(in_value))))
			{
				reset();
				new(_buffer) T(std::forward<U>(in_value));
				_has_value = true;
				return *this;
			}

			template<class... U>
			constexpr voidable_optional& emplace(U&&... args) noexcept(noexcept(new(_buffer) T(std::forward<U>(args)...)))
			{
				reset();
				new(_buffer) T(std::forward<U>(args)...);
				_has_value = true;
				return *this;
			}

			constexpr bool has_value() const noexcept
			{
				return _has_value;
			}

			constexpr T& value() noexcept
			{
				assert(_has_value);
				return *(T*)_buffer;
			}

			constexpr const T& get_value() const noexcept
			{
				assert(_has_value);
				return *(const T*)_buffer;
			}
			
			constexpr bool operator ==(const voidable_optional& rhs) const noexcept(noexcept(get_value() == rhs.value()))
			{
				if (_has_value != rhs._has_value)
				{
					return false;
				}
				else if (!_has_value)
				{
					return true;
				}
				else
				{
					return get_value() == rhs.get_value();
				}
			}

			constexpr T& operator *() noexcept
			{
				return get_value();
			}

			constexpr const T& operator *() const noexcept
			{
				return get_value();
			}

			constexpr voidable_optional& operator =(const voidable_optional& rhs) noexcept(noexcept(new(_buffer) T(rhs.value())))
			{
				reset();
				_has_value = rhs._has_value;
				if (rhs._has_value)
				{
					new(_buffer) T(rhs.get_value());
				}
				return *this;
			}

			constexpr voidable_optional& operator =(voidable_optional&& rhs) noexcept
			{
				reset();
				_has_value = rhs._has_value;
				if (rhs._has_value)
				{
					new(_buffer) T(std::move(rhs.get_value()));
					rhs._has_value = false;
				}
				return *this;
			}

			constexpr T* operator ->() noexcept
			{
				return &get_value();
			}

			constexpr const T* operator ->() const noexcept
			{
				return &get_value();
			}
		};

		template<class T>
		voidable_optional(T&&) -> voidable_optional<std::remove_const_t<std::remove_reference_t<T>>>;

		template<>
		class voidable_optional<void>
		{
			bool _has_value;

		private:
			constexpr voidable_optional(int) noexcept
				: _has_value(true)
			{
			}

		public:
			constexpr voidable_optional() noexcept
				: _has_value(false)
			{
			}

			constexpr voidable_optional(std::nullopt_t) noexcept
				: _has_value(false)
			{
			}

			constexpr voidable_optional(const voidable_optional& rhs) noexcept
				: _has_value(rhs._has_value)
			{
			}

			constexpr voidable_optional(voidable_optional&& rhs) noexcept
				: _has_value(rhs._has_value)
			{
				rhs._has_value = false;
			}

			~voidable_optional() noexcept = default;

			constexpr voidable_optional& reset() noexcept
			{
				_has_value = false;
				return *this;
			}

			constexpr voidable_optional& set_value() noexcept
			{
				_has_value = true;
				return *this;
			}

			constexpr voidable_optional& emplace() noexcept
			{
				_has_value = true;
				return *this;
			}

			constexpr bool has_value() const noexcept
			{
				return _has_value;
			}

			constexpr void get_value() const noexcept
			{
				assert(_has_value);
			}

			constexpr bool operator ==(const voidable_optional& rhs) const noexcept
			{
				return _has_value == rhs._has_value;
			}

			static constexpr voidable_optional fill() noexcept
			{
				return voidable_optional(0);
			}

			static constexpr voidable_optional none() noexcept
			{
				return voidable_optional();
			}
		};
	}

	class task_canceled_exception : public std::exception
	{
	public:
		task_canceled_exception() noexcept
			: std::exception("The task was canceled.")
		{
		}
	};

	enum class task_status
	{
		created,
		running,
		ran_to_completion,
		faulted,
		canceled
	};

	namespace details
	{
		class suspend_and_destroy_if
		{
			const bool _suspend_and_destroy;

		public:
			inline constexpr suspend_and_destroy_if(bool suspend_and_destroy) noexcept
				: _suspend_and_destroy(suspend_and_destroy)
			{
			}

			inline constexpr bool await_ready() const noexcept
			{
				return !_suspend_and_destroy;
			}

			void await_suspend(std::coroutine_handle<> coro) const noexcept
			{
				if (_suspend_and_destroy)
				{
					coro.destroy();
				}
			}

			inline constexpr void await_resume() const noexcept
			{
			}

			inline explicit constexpr operator bool() const noexcept
			{
				return _suspend_and_destroy;
			}
		};

		class awaiter_base : public std::enable_shared_from_this<awaiter_base>
		{
		protected:
			inline awaiter_base() noexcept
			{
			}

		public:
			virtual ~awaiter_base() noexcept
			{
			}

			bool await_ready() noexcept
			{
				return is_completed();
			}

			template<class TCoroutineHandle>
			void await_suspend(TCoroutineHandle&& coro)
			{
				continue_with([this, coro, awaiter = coro.promise().get_awaiter()](auto other_awaiter)
				{
					if (other_awaiter->is_cancellation_requested())
					{
						awaiter->cancel();
					}
					
					coro.resume();
				});
			}

			void await_resume()
			{
				wait();
				if (auto exception_ptr = get_exception())
				{
					std::rethrow_exception(exception_ptr);
				}
			}

			virtual task_status get_status() const noexcept = 0;
			virtual void continue_with(std::move_only_function<void(std::shared_ptr<awaiter_base>)> continuation_body) = 0;
			virtual std::exception_ptr get_exception() = 0;
			virtual void wait() noexcept = 0;
			virtual bool wait_for(const std::chrono::nanoseconds& timeout) noexcept = 0;
			virtual bool wait_until(const std::chrono::steady_clock::time_point& timeout) noexcept = 0;

			virtual void cancel() = 0;
			virtual void set_result() { throw std::runtime_error("A typed task cannot setting result without value."); }
			virtual bool set_exception(std::exception_ptr in_exception_ptr) = 0;

			virtual suspend_and_destroy_if add_stop_token(std::stop_token s_token) = 0;
			virtual void add_stop_callback(std::stop_token s_token, std::move_only_function<void()> callback_body) = 0;
			virtual bool is_cancellation_requested() const noexcept = 0;

			inline bool is_completed() const noexcept
			{
				auto status = get_status();
				return status == task_status::ran_to_completion ||
					status == task_status::faulted ||
					status == task_status::canceled;
			}
		};

		template<class T>
		class awaiter : public awaiter_base
		{
			template<class U>
			friend class awaiter;
			using callback_t = std::stop_callback<std::move_only_function<void()>>;

			std::mutex _lock;
			std::condition_variable _future;

			voidable_optional<T> _promise;
			task_status _status;
			std::exception_ptr _exception_ptr;

			std::vector<std::move_only_function<void(std::shared_ptr<awaiter_base>)>> _thens;
			std::vector<std::unique_ptr<callback_t>> _stop_callbacks;

		public:
			awaiter(std::stop_token s_token = {}, task_status initial_status = task_status::running)
				: _status(initial_status)
			{
				if (s_token.stop_possible())
				{
					add_stop_token(s_token);
				}
			}

			awaiter(const awaiter&) = delete;

			~awaiter() noexcept
			{
			}

			decltype(auto) await_resume()
			{
				return get_result();
			}

			virtual task_status get_status() const noexcept override
			{
				return _status;
			}

			virtual void continue_with(std::move_only_function<void(std::shared_ptr<awaiter_base>)> continuation_body) override
			{
				std::unique_lock<std::mutex> scoped_lock(_lock);
				if (is_completed())
				{
					scoped_lock.unlock();
					continuation_body(shared_from_this());
				}
				else
				{
					_thens.emplace_back(std::move(continuation_body));
				}
			}

			virtual std::exception_ptr get_exception() override
			{
				return _exception_ptr;
			}

			T get_result()
			{
				wait();
				if (_exception_ptr)
				{
					std::rethrow_exception(_exception_ptr);
				}
				return _promise.get_value();
			}

			virtual void wait() noexcept override
			{
				if (is_completed())
				{
					return;
				}

				std::unique_lock<std::mutex> scoped_lock(_lock);
				_future.wait(scoped_lock, [this] { return is_completed(); });
			}

			virtual bool wait_for(const std::chrono::nanoseconds& timeout) noexcept override
			{
				if (is_completed())
				{
					return true;
				}

				std::unique_lock<std::mutex> scoped_lock(_lock);
				return _future.wait_for(scoped_lock, timeout, [this] { return is_completed(); });
			}

			virtual bool wait_until(const std::chrono::steady_clock::time_point& timeout) noexcept override
			{
				if (is_completed())
				{
					return true;
				}

				std::unique_lock<std::mutex> scoped_lock(_lock);
				return _future.wait_until(scoped_lock, timeout, [this] { return is_completed(); });
			}

			virtual void cancel() override
			{
				std::unique_lock<std::mutex> scoped_lock(_lock);
				if (!is_completed())
				{
					_status = task_status::canceled;
					_exception_ptr = std::make_exception_ptr(task_canceled_exception());

					auto local_thens = std::move(_thens);
					_future.notify_all();
					scoped_lock.unlock();
					this->invoke_thens(std::move(local_thens));
				}
			}

			virtual bool set_exception(std::exception_ptr in_exception_ptr) override
			{
				std::unique_lock<std::mutex> scoped_lock(_lock);
				if (is_cancellation_requested())
				{
					scoped_lock.unlock();
					cancel();
					return false;
				}

				if (!is_completed())
				{
					_exception_ptr = in_exception_ptr;
					try
					{
						std::rethrow_exception(in_exception_ptr);
					}
					catch (const task_canceled_exception&)
					{
						_status = task_status::canceled;
					}
					catch (...)
					{
						_status = task_status::faulted;
					}

					auto local_thens = std::move(_thens);
					_future.notify_all();
					scoped_lock.unlock();
					this->invoke_thens(std::move(local_thens));
					return true;
				}
				else
				{
					throw std::runtime_error("Task already completed.");
				}
			}

			virtual suspend_and_destroy_if add_stop_token(std::stop_token s_token) override
			{
				if (!s_token.stop_possible())
				{
					return is_cancellation_requested();
				}

				if (s_token.stop_requested())
				{
					cancel();
					return true;
				}

				std::unique_lock<std::mutex> scoped_lock(_lock);
				if (is_cancellation_requested())
				{
					return true;
				}

				if (is_completed())
				{
					return false;
				}

				add_cancellation_token(std::move(s_token));
				return is_cancellation_requested();
			}

			virtual void add_stop_callback(std::stop_token s_token, std::move_only_function<void()> callback_body) override
			{
				std::unique_lock<std::mutex> scoped_lock(_lock);
				_stop_callbacks.emplace_back(std::make_unique<callback_t>(
					s_token,
					std::move(callback_body))
				);
			}

			virtual bool is_cancellation_requested() const noexcept override
			{
				if (is_canceled())
				{
					return true;
				}

				return false;
			}

			inline bool is_completed() const noexcept
			{
				auto status = get_status();
				return status == task_status::ran_to_completion ||
					status == task_status::faulted ||
					status == task_status::canceled;
			}

			inline bool is_completed_successfully() const noexcept
			{
				return _status == task_status::ran_to_completion;
			}

			inline bool is_canceled() const noexcept
			{
				return _status == task_status::canceled;
			}

			inline bool is_faulted() const noexcept
			{
				return _status == task_status::faulted;
			}

			template<class... U>
			void set_result(U&&... args) requires
				(std::same_as<T, void> && sizeof...(U) == 0) ||
				(!std::same_as<T, void> && std::constructible_from<T, U...>)
			{
				std::unique_lock<std::mutex> scoped_lock(_lock);
				if (is_cancellation_requested())
				{
					scoped_lock.unlock();
					cancel();
					return;
				}

				if (!is_completed())
				{
					_promise.emplace(std::forward<U>(args)...);
					_status = task_status::ran_to_completion;

					auto local_thens = std::move(_thens);
					_future.notify_all();
					scoped_lock.unlock();
					this->invoke_thens(std::move(local_thens));
				}
				else
				{
					throw std::runtime_error("Task already completed.");
				}
			}

		private:
			void add_cancellation_token(std::stop_token s_token)
			{
				_stop_callbacks.emplace_back(std::make_unique<callback_t>(
					s_token,
					[this]() {
						this->cancel();
					})
				);
			}

			void invoke_thens(std::vector<std::move_only_function<void(std::shared_ptr<awaiter_base>)>> thens)
			{
				for (auto& then : thens)
				{
					this->invoke(then);
				}
			}

			void invoke(std::move_only_function<void(std::shared_ptr<awaiter_base>)>& invoke)
			{
				invoke(shared_from_this());
			}
		};

		template<class Task_t>
		struct awaiter_trait
		{
			using type = Task_t::awaiter_t;
		};

		template<>
		struct awaiter_trait<void>
		{
			using type = void;
		};

		template<class T>
		class wrap_shared_awaiter
		{
			std::shared_ptr<T> _awaiter;

		public:
			inline wrap_shared_awaiter(std::shared_ptr<T> awaiter) noexcept
				: _awaiter(std::move(awaiter))
			{
			}

			inline operator std::shared_ptr<T>() const& noexcept
			{
				return _awaiter;
			}

			inline operator std::shared_ptr<T>() const&& noexcept
			{
				return std::move(_awaiter);
			}

			inline bool await_ready() const noexcept(noexcept(_awaiter->await_ready()))
			{
				return _awaiter->await_ready();
			}

			template<class TCoroutineHandle>
			inline void await_suspend(TCoroutineHandle&& coro) const noexcept(noexcept(_awaiter->await_suspend(std::forward<TCoroutineHandle>(coro))))
			{
				_awaiter->await_suspend(std::forward<TCoroutineHandle>(coro));
			}

			inline decltype(auto) await_resume() const noexcept(noexcept(_awaiter->await_resume()))
			{
				return _awaiter->await_resume();
			}

			inline T* operator ->() const noexcept
			{
				return _awaiter.get();
			}
		};

		template<class T, class Task_t>
		class promise_type_base
		{
		public:
			using awaiter_t = typename awaiter_trait<Task_t>::type;
			using task_t = Task_t;

		private:
			std::shared_ptr<awaiter_t> _awaiter;

		protected:
			template<class... TArgs>
			constexpr promise_type_base(TArgs&&... in_args) noexcept
			{
				if constexpr (!std::same_as<awaiter_t, void>)
				{
					_awaiter = std::make_shared<awaiter_t>(std::forward<TArgs>(in_args)...);
				}
			}

		public:
			awaiter_t* get_awaiter() const noexcept requires (!std::same_as<awaiter_t, void>)
			{
				return _awaiter.get();
			}

			constexpr task_t get_return_object() noexcept requires (!std::same_as<task_t, void>)
			{
				return task_t(_awaiter);
			}

			constexpr void get_return_object() noexcept requires std::same_as<task_t, void>
			{
			}

			constexpr void unhandled_exception()
			{
				if constexpr (std::same_as<awaiter_t, void>)
				{
					std::rethrow_exception(std::current_exception());
				}
				else
				{
					_awaiter->set_exception(std::current_exception());
				}
			}

			constexpr auto initial_suspend() noexcept
			{
				return std::suspend_never();
			}

			constexpr auto final_suspend() noexcept
			{
				return std::suspend_never();
			}

			template<class TAwaitableTask>
			constexpr decltype(auto) await_transform(TAwaitableTask&& in_task)
			{
				return wrap_shared_awaiter(in_task.get_awaiter());
			}
		};

		template<class T, class Task_t>
		class promise_type : public promise_type_base<T, Task_t>
		{
		public:
			promise_type()
			{
			}

			template<class U>
			void return_value(U&& in_value) requires std::constructible_from<T, U>
			{
				this->get_awaiter()->set_result(T(std::forward<U>(in_value)));
			}

			void return_value(const T& in_value)
			{
				this->get_awaiter()->set_result(in_value);
			}

			void return_value(T&& in_value)
			{
				this->get_awaiter()->set_result(std::move(in_value));
			}
		};

		template<class Task_t> requires
			requires { std::declval<Task_t>().get_awaiter()->set_result(); }
		class promise_type<void, Task_t> : public promise_type_base<void, Task_t>
		{
		public:
			promise_type()
			{
			}

			void return_void()
			{
				this->get_awaiter()->set_result();
			}
		};

		class async_void_promise_type : public promise_type_base<void, void>
		{
			bool _cancellation_requested = false;
			std::vector<std::unique_ptr<std::stop_callback<std::move_only_function<void()>>>> _stop_callbacks;

		public:
			inline async_void_promise_type() noexcept
			{
			}

			inline void return_void() noexcept
			{
			}

			inline async_void_promise_type* get_awaiter() noexcept
			{
				return this;
			}

			inline void cancel() noexcept
			{
				_cancellation_requested = true;
			}

			inline bool is_cancellation_requested() const noexcept
			{
				return _cancellation_requested;
			}

			suspend_and_destroy_if add_stop_token(std::stop_token s_token) noexcept
			{
				if (!s_token.stop_possible())
				{
					return is_cancellation_requested();
				}

				if (s_token.stop_requested())
				{
					cancel();
					return true;
				}

				if (is_cancellation_requested())
				{
					return true;
				}

				_stop_callbacks.emplace_back(std::make_unique<std::stop_callback<std::move_only_function<void()>>>(
					s_token,
					[this]() {
						this->cancel();
					})
				);

				return is_cancellation_requested();
			}
		};

		void queue_user_work_item(std::move_only_function<void()> work_item);
		void queue_delayed_user_work_item(std::chrono::nanoseconds delay, std::move_only_function<void()> work_item);
	}

	template<class T = void>
	class [[nodiscard]] task
	{
		template<class U>
		friend class task;

	public:
		using promise_type = ::threading::tasks::details::promise_type<T, task<T>>;
		using awaiter_t = ::threading::tasks::details::awaiter<T>;
		using value_t = T;

	private:
		static bool _configure_default;
		std::shared_ptr<details::awaiter_base> _awaiter;

	public:
		task() = default;
		task(const task&) = default;

		template<class U>
		explicit task(std::shared_ptr<U> in_awaiter) requires
			std::constructible_from<task, std::shared_ptr<U>, int>
			: task(in_awaiter, 0)
		{
		}

		task(std::shared_ptr<awaiter_t> in_awaiter, int)
			: _awaiter(std::move(in_awaiter))
		{
		}

		explicit task(std::shared_ptr<details::awaiter_base> in_awaiter, short)
			: _awaiter(in_awaiter)
		{
		}

		explicit task(const task<>& in_task) requires (!std::same_as<T, void>)
			: task(in_task.get_awaiter())
		{
		}

		inline bool is_valid() const noexcept
		{
			return (bool)_awaiter;
		}

		inline std::shared_ptr<details::awaiter_base> get_awaiter() const requires std::is_void_v<T>
		{
			return _awaiter;
		}

		inline std::shared_ptr<awaiter_t> get_awaiter() const requires (!std::is_void_v<T>)
		{
			return std::static_pointer_cast<awaiter_t>(_awaiter);
		}

		inline task_status get_status() const noexcept
		{
			return _awaiter->get_status();
		}

		inline std::exception_ptr get_exception() const noexcept
		{
			return _awaiter->get_exception();
		}

		inline void rethrow_exception() const
		{
			std::rethrow_exception(_awaiter->get_exception());
		}

		inline void add_stop_callback(std::stop_token s_token, std::move_only_function<void()> callback_body)
		{
			_awaiter->add_stop_callback(s_token, std::move(callback_body));
		}

		template<class TBody>
		auto then(TBody&& continuation_body, std::stop_token s_token = {}) const -> task<std::invoke_result_t<TBody, task>>
		{
			using U = std::invoke_result_t<TBody, task>;
			std::shared_ptr u_awaiter = std::make_shared<details::awaiter<U>>(s_token);
			_awaiter->continue_with([continuation_body = std::forward<TBody>(continuation_body), u_awaiter](std::shared_ptr<details::awaiter_base> result)
			{
				try
				{
					if constexpr (std::is_void_v<U>)
					{
						continuation_body(task(result));
						u_awaiter->set_result();
					}
					else
					{
						auto r = continuation_body(task(result));
						u_awaiter->set_result(std::move(r));
					}
				}
				catch (...)
				{
					u_awaiter->set_exception(std::current_exception());
				}
			});
			return task<U>(u_awaiter);
		}

		inline void wait() const noexcept
		{
			_awaiter->wait();
		}

		inline task<T> wait_async(std::stop_token s_token) const noexcept
		{
			return then([](task t)
			{
				return t.get_result();
			}, s_token);
		}

		inline bool wait_for(const std::chrono::nanoseconds& timeout) const noexcept
		{
			return _awaiter->wait_for(timeout);
		}

		inline T get_result() const
		{
			if constexpr (std::is_void_v<T>)
			{
				_awaiter->await_resume();
			}
			else
			{
				return std::static_pointer_cast<awaiter_t>(_awaiter)->get_result();
			}
		}

		inline bool is_completed() const noexcept
		{
			return _awaiter && _awaiter->is_completed();
		}

		inline bool is_completed_successfully() const noexcept
		{
			return _awaiter && _awaiter->get_status() == task_status::ran_to_completion;
		}

		inline bool is_canceled() const noexcept
		{
			return _awaiter && _awaiter->get_status() == task_status::canceled;
		}

		inline bool is_faulted() const noexcept
		{
			return _awaiter && _awaiter->get_status() == task_status::faulted;
		}

		task& operator =(const task&) = default;
		task& operator =(task&&) = default;

		template<class U>
		explicit operator task<U>() const requires std::same_as<T, void>
		{
			return task<U>(_awaiter);
		}

		template<class U>
		operator task<U>() const requires
			std::same_as<U, void> &&
			(!std::same_as<T, void>)
		{
			return task<U>(_awaiter);
		}

		auto operator <=>(const task&) const = default;
		bool operator ==(const task&) const = default;

	public:
		template<class TBody>
		static auto run(TBody&& body, std::stop_token s_token = {}) -> task<std::invoke_result_t<TBody>>
		{
			static_assert(std::is_void_v<T>, "Use task<>::run instead.");
			
			using U = std::invoke_result_t<TBody>;
			std::shared_ptr u_awaiter = std::make_shared<details::awaiter<U>>(s_token);

			details::queue_user_work_item([u_awaiter, body = std::forward<TBody>(body)]() mutable
			{
				try
				{
					if constexpr (std::is_void_v<U>)
					{
						body();
						u_awaiter->set_result();
					}
					else
					{
						auto r = body();
						u_awaiter->set_result(std::move(r));
					}
				}
				catch (...)
				{
					u_awaiter->set_exception(std::current_exception());
				}
			});
			return task<U>(u_awaiter);
		}

		template<class TBody>
		static task<std::invoke_result_t<TBody>> start_new(TBody&& body, std::stop_token s_token = {});

		static auto yield()
		{
			static_assert(std::is_void_v<T>, "Use task<>::yield instead.");

			std::shared_ptr u_awaiter = std::make_shared<details::awaiter<void>>();
			details::queue_user_work_item([u_awaiter]
			{
				u_awaiter->set_result();
			});

			return task<>(std::move(u_awaiter));
		}

		static task<> delay(std::chrono::nanoseconds delay, std::stop_token s_token = {})
		{
			static_assert(std::is_void_v<T>, "Use task<>::delay instead.");

			std::shared_ptr u_awaiter = std::make_shared<details::awaiter<void>>(s_token);
			details::queue_delayed_user_work_item(delay, [u_awaiter]() mutable
			{
				u_awaiter->set_result();
			});

			return task<>(std::move(u_awaiter));
		}

		static task<> completed_task()
		{
			static_assert(std::is_void_v<T>, "Use task<>::completed_task instead.");

			static thread_local std::shared_ptr s_awaiter = []
			{
				auto ptr = std::make_shared<details::awaiter<void>>();
				ptr->set_result();
				return ptr;
			}();

			return task<>(s_awaiter);
		}

		template<class U> requires (!std::same_as<U, void>)
		static task<U> from_result(U in_value)
		{
			static_assert(std::is_void_v<T>, "Use task<>::from_reuslt<U> instaed.");

			auto u_awaiter = std::make_shared<details::awaiter<U>>();
			u_awaiter->set_result(std::move(in_value));
			return task<U>(u_awaiter);
		}
	};

	template<class T = void>
	class task_completion_source
	{
		template<class>
		friend class task_completion_source;

		std::shared_ptr<details::awaiter<T>> _awaiter;

	private:
		task_completion_source(std::shared_ptr<details::awaiter<T>> in_awaiter)
			: _awaiter(std::move(in_awaiter))
		{
		}

	public:
		task_completion_source() = default;
		task_completion_source(const task_completion_source&) = default;
		task_completion_source(task_completion_source&&) = default;

		bool is_valid() const noexcept { return (bool)_awaiter; }

		void set_result() const
		{
			this->set_result_impl();
		}

		template<class U>
		void set_result(U&& result) const
		{
			this->set_result_impl(std::forward<U>(result));
		}

		template<class TException>
		void set_exception(const TException& exception_obj) const
		{
			this->set_exception(std::make_exception_ptr(exception_obj));
		}

		void set_exception(std::exception_ptr exception_ptr) const
		{
			xassert(is_valid());
			_awaiter->set_exception(std::move(exception_ptr));
		}

		void set_canceled() const
		{
			xassert(is_valid());
			_awaiter->cancel();
		}

		task<T> get_task() const
		{
			xassert(is_valid());
			return task<T>(_awaiter);
		}

		task_completion_source& operator =(const task_completion_source&) = default;
		task_completion_source& operator =(task_completion_source&&) = default;

		template<class U = T>
		static task_completion_source<U> create(std::stop_token s_token = {})
		{
			return task_completion_source<U>(std::make_shared<details::awaiter<U>>(s_token));
		}

	private:
		template<class... U>
		void set_result_impl(U&&... args) const
		{
			xassert(is_valid());
			_awaiter->set_result(std::forward<U>(args)...);
		}

		void xassert(bool condition) const
		{
			if (!condition)
			{
				throw std::runtime_error("Invalid operation on an invalid task_completion_source.");
			}
		}
	};

	template<class T>
	template<class TBody>
	task<std::invoke_result_t<TBody>> task<T>::start_new(TBody&& body, std::stop_token s_token)
	{
		using result_t = std::invoke_result_t<TBody>;
		auto tcs = task_completion_source<>::create<result_t>(s_token);
		std::thread([tcs, body = std::forward<TBody>(body)]() mutable
		{
			try
			{
				if constexpr (std::is_void_v<T>)
				{
					body();
					tcs.set_result();
				}
				else
				{
					auto r = body();
					tcs.set_result(std::move(r));
				}
			}
			catch (...)
			{
				tcs.set_exception(std::current_exception());
			}
		}).detach();

		return tcs.get_task();
	}
}

template<class TOwningClass, class... TArgs>
struct std::coroutine_traits<void, TOwningClass&, TArgs...>
{
	using promise_type = threading::tasks::details::async_void_promise_type;
};

template<class... TArgs>
struct std::coroutine_traits<void, TArgs...>
{
	using promise_type = threading::tasks::details::async_void_promise_type;
};