#pragma once

namespace cora
{
namespace reflection
{
    struct processor2
    {
        
    };

} // namespace reflection
} // namespace cora

template<typename processor, typename T>                               
void reflect(processor && proc, T && object)
{
    reflect2(std::forward<processor>(proc), std::forward<T>(object), std::forward<T>(object));
}

#define REFL_STRUCT_BODY(...)                           \
void reflect2(processor && proc, __VA_ARGS__  const & lhs, __VA_ARGS__  const & rhs) \
{                                                       \
    typedef __VA_ARGS__ type;                           \
                                                        \
    type& lobj = const_cast<type&>(lhs);/*small hack*/  \
    type& robj = const_cast<type&>(rhs);/*small hack*/  \
                                                        \
    (void) proc; /*avoiding warning*/                   \
    (void) lobj; /*avoiding warning*/                   \
    (void) robj; /*avoiding warning*/                   


    // for using outside structure or class
#define REFL_STRUCT(type)                               \
template<class processor>                               \
    REFL_STRUCT_BODY(type)

// for using inside structure or class
#define REFL_INNER(type)                        \
template<class processor>                       \
    friend REFL_STRUCT_BODY(type)

#define REFL_ENTRY_NAMED(name, entry)           \
    if constexpr (std::is_base_of_v<cora::reflection::processor2, processor>) \
        proc(name, lobj.entry, robj.entry); \
    else \
        proc(name, lobj.entry);

#define REFL_ENTRY(entry)                       \
    REFL_ENTRY_NAMED(#entry, entry)

#define REFL_ENTRY2(entry1, entry2)             \
    REFL_ENTRY(entry1) \
    REFL_ENTRY(entry2) 

#define REFL_ENTRY3(entry1, entry2, entry3)             \
    REFL_ENTRY(entry1) \
    REFL_ENTRY(entry2) \
    REFL_ENTRY(entry3) 

#define REFL_ENTRY4(entry1, entry2, entry3, entry4)             \
    REFL_ENTRY(entry1) \
    REFL_ENTRY(entry2) \
    REFL_ENTRY(entry3) \
    REFL_ENTRY(entry4) 

#define REFL_CHAIN(base)                        \
    static_assert(std::is_base_of_v<base, type>, "REFL_CHAIN for non-base type"); \
    static_assert(!std::is_same_v<base, type>, "REFL_CHAIN for same type"); \
    reflect2(std::forward<processor>(proc), static_cast<base&>(lobj), static_cast<base&>(robj));

#define REFL_END()                              \
}

