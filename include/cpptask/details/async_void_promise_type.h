// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <cassert>
#include <coroutine>

namespace cpptask::details
{
    class async_void_promise_type
    {
    public:
        constexpr void get_return_object() const noexcept
        {
        }

        void unhandled_exception()
        {
            assert(false && "Unhandled exception in async void task");
        }

        constexpr auto initial_suspend() const noexcept
        {
            return std::suspend_never();
        }

        constexpr auto final_suspend() const noexcept
        {
            return std::suspend_never();
        }

        constexpr void return_void() const noexcept
        {
        }

        template<class AwaitableTask>
        constexpr decltype(auto) await_transform(AwaitableTask&& task)
        {
            return task.get_awaiter();
        }
    };
}