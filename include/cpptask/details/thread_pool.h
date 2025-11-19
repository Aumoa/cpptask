// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <functional>
#include <chrono>

namespace cpptask::details
{
    class thread_pool
    {
    public:
      template<class T>
#if __cpp_lib_move_only_function
      using function_t = std::move_only_function<T>;
#else
      using function_t = std::function<T>;
#endif

    public:
      static void queue_user_work_item(function_t<void()> continuation);
      static void queue_delayed_user_work_item(std::chrono::nanoseconds delay, function_t<void()> continuation);
    };
}