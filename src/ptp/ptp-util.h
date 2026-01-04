#pragma once

#include "mlib/mlib.h"
#include "ptp/ptp-const.h"

PTP_EXPORT void PTP_InitDefaultAllocator(MAllocator* allocator);
PTP_EXPORT void PTP_MemIOFree(MMemIO* memIO);
PTP_EXPORT void PTP_StrFree(MAllocator* allocator, MStr* str);
