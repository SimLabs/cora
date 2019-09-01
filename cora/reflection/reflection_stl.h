#pragma once

#include "cora/reflection/reflection.h"

template<typename processor, typename T1, typename T2>
REFL_STRUCT_BODY(std::pair<T1, T2>)
    REFL_ENTRY(first)
    REFL_ENTRY(second)
REFL_END()