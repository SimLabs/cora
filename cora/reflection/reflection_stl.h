#pragma once

#include "cora/reflection/reflection.h"

/*template<typename processor, typename T1, typename T2>
REFL_STRUCT_BODY(std::pair<T1, T2>)
    REFL_ENTRY(first)
    REFL_ENTRY(second)
REFL_END()*/

template<size_t SIZE>
struct reflect_helper
{

    template<std::size_t I = 0, class processor, class tuple_t>
    static typename std::enable_if<I == SIZE, void>::type reflect2_tuple(processor& /* proc */, tuple_t& /* lhs */, tuple_t& /* rhs */)
    {
    }

    template<std::size_t I = 0, class processor, class tuple_t>
    static typename std::enable_if < I < SIZE, void>::type reflect2_tuple(processor& proc, tuple_t& lhs, tuple_t& rhs)
    {
        cora::reflection::apply_proc(proc, std::get<I>(lhs), std::get<I>(rhs), std::to_string(I).c_str());
        reflect2_tuple<I + 1>(proc, lhs, rhs);
    }
};

template<class processor, class ...Args>
void reflect2(processor& proc, std::tuple<Args...> const& lhs, std::tuple<Args...> const& rhs)
{
    typedef std::tuple<Args...> type;
    type& lobj = const_cast<type&>(lhs);/*small hack*/
    type& robj = const_cast<type&>(rhs);/*small hack*/

    reflect_helper<std::tuple_size<type>::value>::reflect2_tuple(proc, lobj, robj);
}

template<class processor, class Type, size_t Size>
void reflect2(processor& proc, std::array<Type, Size> const& lhs, std::array<Type, Size> const& rhs)
{
    using type = std::array<Type, Size>;
    type& lobj = const_cast<type&>(lhs);/*small hack*/
    type& robj = const_cast<type&>(rhs);/*small hack*/

    for (size_t i = 0; i < Size; ++i)
        cora::reflection::apply_proc(proc, lobj[i], robj[i], std::to_string(i).c_str());
}
