// Copyright 2020-2025 Aumoa. All right reserved.

#pragma once

#include <optional>
#include <cassert>

namespace cpptask::details
{
    template<class T>
    class voidable_optional
    {
        bool _has_value;
        char* _buffer[sizeof(T)];

    public:
        constexpr voidable_optional() noexcept
            : _has_value(false)
#ifndef NDEBUG
            , _buffer{}
#endif
        {
        }

        constexpr voidable_optional(std::nullopt_t) noexcept
            : _has_value(false)
#ifndef NDEBUG
            , _buffer{}
#endif
        {
        }

        template<class U>
        constexpr voidable_optional(U&& in_value) noexcept
            : _has_value(true)
        {
            new(_buffer) T(std::forward<U>(in_value));
        }

        constexpr voidable_optional(const voidable_optional& rhs) noexcept(noexcept(new(_buffer) T(rhs.value())))
            : _has_value(rhs._has_value)
        {
            if (rhs._has_value)
            {
                new(_buffer) T(rhs.get_value());
            }
        }

        constexpr voidable_optional(voidable_optional&& rhs) noexcept
            : _has_value(rhs._has_value)
        {
            if (rhs._has_value)
            {
                new(_buffer) T(std::move(rhs.get_value()));
                rhs._has_value = false;
            }
        }

        constexpr ~voidable_optional() noexcept
        {
            reset();
        }

        constexpr voidable_optional& reset() noexcept
        {
            if (_has_value)
            {
                get_value().~T();
                _has_value = false;
            }
            return *this;
        }

        template<class U>
        constexpr voidable_optional& set_value(U&& in_value) noexcept(noexcept(new(_buffer) T(std::forward<U>(in_value))))
        {
            reset();
            new(_buffer) T(std::forward<U>(in_value));
            _has_value = true;
            return *this;
        }

        template<class... U>
        constexpr voidable_optional& emplace(U&&... args) noexcept(noexcept(new(_buffer) T(std::forward<U>(args)...)))
        {
            reset();
            new(_buffer) T(std::forward<U>(args)...);
            _has_value = true;
            return *this;
        }

        constexpr bool has_value() const noexcept
        {
            return _has_value;
        }

        constexpr T& value() noexcept
        {
            assert(_has_value);
            return *(T*)_buffer;
        }

        constexpr const T& get_value() const noexcept
        {
            assert(_has_value);
            return *(const T*)_buffer;
        }
        
        constexpr bool operator ==(const voidable_optional& rhs) const noexcept(noexcept(get_value() == rhs.value()))
        {
            if (_has_value != rhs._has_value)
            {
                return false;
            }
            else if (!_has_value)
            {
                return true;
            }
            else
            {
                return get_value() == rhs.get_value();
            }
        }

        constexpr T& operator *() noexcept
        {
            return get_value();
        }

        constexpr const T& operator *() const noexcept
        {
            return get_value();
        }

        constexpr voidable_optional& operator =(const voidable_optional& rhs) noexcept(noexcept(new(_buffer) T(rhs.value())))
        {
            reset();
            _has_value = rhs._has_value;
            if (rhs._has_value)
            {
                new(_buffer) T(rhs.get_value());
            }
            return *this;
        }

        constexpr voidable_optional& operator =(voidable_optional&& rhs) noexcept
        {
            reset();
            _has_value = rhs._has_value;
            if (rhs._has_value)
            {
                new(_buffer) T(std::move(rhs.get_value()));
                rhs._has_value = false;
            }
            return *this;
        }

        constexpr T* operator ->() noexcept
        {
            return &get_value();
        }

        constexpr const T* operator ->() const noexcept
        {
            return &get_value();
        }
    };

    template<class T>
    voidable_optional(T&&) -> voidable_optional<std::remove_const_t<std::remove_reference_t<T>>>;

    template<>
    class voidable_optional<void>
    {
        bool _has_value;

    private:
        constexpr voidable_optional(int) noexcept
            : _has_value(true)
        {
        }

    public:
        constexpr voidable_optional() noexcept
            : _has_value(false)
        {
        }

        constexpr voidable_optional(std::nullopt_t) noexcept
            : _has_value(false)
        {
        }

        constexpr voidable_optional(const voidable_optional& rhs) noexcept
            : _has_value(rhs._has_value)
        {
        }

        constexpr voidable_optional(voidable_optional&& rhs) noexcept
            : _has_value(rhs._has_value)
        {
            rhs._has_value = false;
        }

        ~voidable_optional() noexcept = default;

        constexpr voidable_optional& reset() noexcept
        {
            _has_value = false;
            return *this;
        }

        constexpr voidable_optional& set_value() noexcept
        {
            _has_value = true;
            return *this;
        }

        constexpr voidable_optional& emplace() noexcept
        {
            _has_value = true;
            return *this;
        }

        constexpr bool has_value() const noexcept
        {
            return _has_value;
        }

        constexpr void get_value() const noexcept
        {
            assert(_has_value);
        }

        constexpr bool operator ==(const voidable_optional& rhs) const noexcept
        {
            return _has_value == rhs._has_value;
        }

        static constexpr voidable_optional fill() noexcept
        {
            return voidable_optional(0);
        }

        static constexpr voidable_optional none() noexcept
        {
            return voidable_optional();
        }
    };
}