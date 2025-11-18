// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

namespace cpptask
{
    enum class task_status
    {
        created,
        running,
        ran_to_completion,
        faulted,
        canceled
    };
}