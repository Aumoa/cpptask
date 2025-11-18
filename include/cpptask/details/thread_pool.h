// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <functional>
#include <chrono>

namespace cpptask::details
{
    class thread_pool
    {
    public:
      static void queue_user_work_item(std::move_only_function<void()> continuation);
      static void queue_delayed_user_work_item(std::chrono::nanoseconds delay, std::move_only_function<void()> continuation);
    };
}