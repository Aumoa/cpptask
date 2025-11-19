// Copyright 2020-2025 Aumoa.lib. All right reserved.

#include "cpptask/aggregate_exception.h"

namespace cpptask
{
    aggregate_exception::aggregate_exception(std::vector<std::exception_ptr> exception_ptrs)
        : _exception_ptrs{ std::move(exception_ptrs) }
    {
    }

    aggregate_exception::~aggregate_exception() noexcept
    {
    }

    const char* aggregate_exception::what() const noexcept
    {
        return "One or more errors occurred.";
    }

    std::span<const std::exception_ptr> aggregate_exception::get_exceptions() const noexcept
    {
        return _exception_ptrs;
    }
}