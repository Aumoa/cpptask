// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include "cpptask/details/shared_task.h"
#include <memory>

namespace cpptask::details
{
    template<class T>
    class task_awaiter
    {
    private:
        const std::shared_ptr<shared_task<T>> _task;

    public:
        task_awaiter(std::shared_ptr<shared_task<T>> task)
            : _task(std::move(task))
        {
        }

        bool await_ready() noexcept
        {
            return _task->is_completed();
        }

        template<class CoroutineHandle>
        void await_suspend(CoroutineHandle&& coro) noexcept
        {
            _task->then([c = std::forward<CoroutineHandle>(coro)]
            {
                c.resume();
            });
        }

        decltype(auto) await_resume()
        {
            return _task->get_result();
        }
    };
}