// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include "cpptask/details/shared_task.h"
#include <memory>
#include <cassert>
#include <coroutine>
#include <concepts>

namespace cpptask::details
{
    template<class T, class Task>
    class promise_type_base
    {
        std::shared_ptr<shared_task<T>> _task;

    protected:
        promise_type_base()
        {
            _task = std::make_shared<shared_task<T>>();
        }

    protected:
        template<class C>
        shared_task<T>* get_task(this C&& c) noexcept
        {
            return c._task.get();
        }

    public:
        promise_type_base(const promise_type_base&) = delete;

        Task get_return_object() noexcept
        {
            return Task(_task);
        }

        auto unhandled_exception() noexcept
        {
            bool b = _task->try_set_exception(std::current_exception());
            assert(b);
        }

        auto initial_suspend() noexcept
        {
            _task->transit_to_running();
            return std::suspend_never();
        }

        constexpr auto final_suspend() const noexcept
        {
            return std::suspend_never();
        }

        template<class AwaitableTask>
        constexpr decltype(auto) await_transform(AwaitableTask&& task)
        {
            return task.get_awaiter();
        }
    };

    template<class T, class Task>
    class promise_type : public promise_type_base<T, Task>
    {
    public:
        promise_type()
        {
        }

        template<class U>
        void return_value(U&& value) requires std::constructible_from<T, U>
        {
            this->get_task()->set_result(std::forward<U>(value));
        }
    };

    template<class Task>
    class promise_type<void, Task> : public promise_type_base<void, Task>
    {
    public:
        promise_type()
        {
        }

        void return_void()
        {
            this->get_task()->set_result();
        }
    };
}