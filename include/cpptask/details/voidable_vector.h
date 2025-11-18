// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <concepts>
#include <vector>

namespace cpptask::details
{
    template<class T>
    class voidable_vector : public std::vector<T>
    {
    public:
        template<class... TArgs> requires std::constructible_from<std::vector<T>, TArgs...>
        constexpr voidable_vector(TArgs&&... args) noexcept(noexcept(std::vector<T>(std::forward<TArgs>(args)...)))
            : std::vector<T>(std::forward<TArgs>(args)...)
        {
        }

        constexpr voidable_vector& operator =(const voidable_vector& r) const noexcept(noexcept(std::declval<std::vector<T>&>() = std::declval<const std::vector<T>&>()))
        {
            std::vector<T>::operator =(r);
            return *this;
        }

        constexpr voidable_vector&& operator =(voidable_vector&& r) const noexcept(noexcept(std::declval<std::vector<T>&>() = std::declval<std::vector<T>&&>()))
        {
            std::vector<T>::operator =(std::move(r));
            return *this;
        }
    };

    template<>
    class voidable_vector<void>
    {
        size_t _size = 0;

    public:
        constexpr voidable_vector() noexcept = default;
        constexpr voidable_vector(size_t) noexcept {}
        constexpr voidable_vector(const voidable_vector&) noexcept = default;
        constexpr voidable_vector(voidable_vector&&) noexcept = default;

        constexpr voidable_vector& operator =(const voidable_vector&) noexcept = default;
        constexpr voidable_vector& operator =(voidable_vector&&) noexcept = default;

        constexpr void resize(size_t new_size) noexcept { _size = new_size; }
        constexpr size_t size() const noexcept { return _size; }
    };
}