#pragma once

#include "mlib/mlib.h"
#include "aw/aw-const.h"

AW_EXPORT void Aw_InitDefaultAllocator(MAllocator* allocator);
AW_EXPORT void Aw_MemIOFree(MMemIO* memIO);
AW_EXPORT void Aw_StrFree(MAllocator* allocator, MStr* str);
