#include "aw/aw-util.h"
#include "aw/aw-device-list.h"

void Aw_InitDefaultAllocator(MAllocator* allocator) {
    MAllocatorMakeClibHeap(allocator);
}

void Aw_MemIOFree(MMemIO* memIO) {
    MFree(memIO->allocator, memIO->mem, memIO->capacity);
    memIO->size = 0;
    memIO->capacity = 0;
}

void Aw_StrFree(MAllocator* allocator, MStr* str) {
    MStrFree(allocator, *str);
}
