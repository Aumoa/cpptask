// Copyright 2020-2025 Aumoa.lib. All right reserved.

#pragma once

#include <exception>

namespace cpptask
{
    /**
     * @brief Exception thrown when a task is canceled.
     * 
     * This exception is thrown when a task operation is canceled before completion.
     * It derives from std::exception and provides standard exception handling capabilities.
     */
    class task_canceled_exception : public std::exception
    {
    public:
        /**
         * @brief Default constructor.
         */
        task_canceled_exception();
        virtual ~task_canceled_exception() noexcept override;
        
        /**
         * @brief Returns the explanatory string.
         * 
         * @return A C-string describing the exception.
         */
        const char* what() const noexcept override;
    };
}