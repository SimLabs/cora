#pragma once

#include "cora/reflection/reflection.h"

namespace cora
{

    struct reflect_eq_processor
        : cora::reflection::processor2
    {
        reflect_eq_processor()
            : not_eq_(false)
        {}

        template<class T>
        void operator()(T &lhs, T &rhs, char const * /*name*/, ...)
        {
            if (not_eq_) return;
            if (lhs != rhs)
                not_eq_ = true;
        }

        bool get_result() const
        {
            return !not_eq_;
        }

    private:
        bool not_eq_;
    };

    struct reflect_less_processor
        : cora::reflection::processor2
    {
        reflect_less_processor()
            : result_(false)
            , stop_(false)
        {}

        template<class T>
        void operator()(T &lhs, T &rhs, char const * /*name*/, ...)
        {
            if (stop_) return;
            if (lhs == rhs) return;
            stop_ = true;
            result_ = lhs < rhs;
        }

        bool get_result() const
        {
            return result_;
        }

    private:
        bool result_;
        bool stop_;
    };

} // namespace cora

#define ENABLE_REFL_EQ(type)                               \
    friend bool operator==(type const &a, type const &b)   \
    {                                                      \
        cora::reflect_eq_processor proc;              \
        reflect2(proc, a, b);                              \
        return proc.get_result();                          \
    }                                                      \
    friend bool operator!=(type const &a, type const &b)   \
    {                                                      \
        return !(a == b);                                  \
    }

#define ENABLE_REFL_CMP(type)                              \
    ENABLE_REFL_EQ(type)                                   \
    friend bool operator<(type const &a, type const &b)    \
    {                                                      \
        cora::reflect_less_processor proc;            \
        reflect2(proc, a, b);                              \
        return proc.get_result();                          \
    }                                                      \
    friend bool operator<=(type const &a, type const &b)   \
    {                                                      \
        return !(b < a);                                   \
    }                                                      \
    friend bool operator>(type const &a, type const &b)    \
    {                                                      \
        return b < a;                                      \
    }                                                      \
    friend bool operator>=(type const &a, type const &b)   \
    {                                                      \
        return !(a < b);                                   \
    }                                                      