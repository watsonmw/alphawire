#include "ptp/ptp-util.h"
#include "ptp/ptp-device-list.h"

void PTP_InitDefaultAllocator(MAllocator* allocator) {
    MAllocatorMakeClibHeap(allocator);
}

void PTP_MemIOFree(MMemIO* memIO) {
    MFree(memIO->allocator, memIO->mem, memIO->capacity);
    memIO->size = 0;
    memIO->capacity = 0;
}

void PTP_StrFree(MAllocator* allocator, MStr* str) {
    MStrFree(allocator, *str);
}
