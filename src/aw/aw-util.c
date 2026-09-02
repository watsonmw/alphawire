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

void Aw_PtpEventArrayFree(MAllocator* allocator, AwPtpEventArray* array) {
    MFree(allocator, array->data, array->capacity * sizeof(AwPtpEvent));
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
}
