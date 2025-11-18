// Copyright 2020-2025 Aumoa.lib. All right reserved.

#include "cpptask/task_canceled_exception.h"

namespace cpptask
{
    task_canceled_exception::task_canceled_exception()
    {
    }

    task_canceled_exception::~task_canceled_exception() noexcept
    {
    }

    const char* task_canceled_exception::what() const noexcept
    {
        return "Task was canceled.";
    }
}