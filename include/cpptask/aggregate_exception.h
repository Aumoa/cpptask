// Copyright 2020-2025 Aumoa.lib. All right reserved.

#pragma once

#include <exception>
#include <vector>
#include <span>

namespace cpptask
{
    /**
     * @class aggregate_exception
     * @brief Exception class that aggregates multiple exceptions into a single exception object.
     * 
     * This class inherits from std::exception and is used to represent multiple exceptions
     * that occurred during parallel or asynchronous operations. It stores a collection of
     * exception pointers that can be inspected individually.
     */
    class aggregate_exception : public std::exception
    {
    private:
        std::vector<std::exception_ptr> _exception_ptrs;

    public:
        /**p
         * @brief Constructs an aggregate_exception with a collection of exception pointers.
         * @param exception_ptrs A vector of excetion pointers to be aggregated.
         */
        aggregate_exception(std::vector<std::exception_ptr> exception_ptrs);
        virtual ~aggregate_exception() noexcept override;

        /**
         * @brief Returns a C-string describing the exception.
         * 
         * This method overrides the std::exception::what() function to provide
         * a description of the aggregate exception. Currently returns an empty
         * string, but should ideally return a meaningful error message describing
         * the aggregated exceptions.
         * 
         * @return A pointer to a null-terminated string containing the exception description.
         *         Currently returns an empty string.
         * 
         * @note This method is noexcept and guarantees not to throw any exceptions.
         * @note Consider implementing this to return meaningful information about the
         *       aggregated exceptions contained within this exception object.
         */
        virtual const char* what() const noexcept override;

        /**
         * @brief Returns a read-only view of the aggregated exception pointers.
         * @return A std::span containing the exception pointers stored in this aggregate exception.
         * @note This method is noexcept and guarantees not to throw any exceptions.
         */
        std::span<const std::exception_ptr> get_exceptions() const noexcept;
    };
}