#include "mlib.h"

#ifndef M_CLIB_DISABLE
#include <stdlib.h>
#endif

// Define to log allocations
// #define M_LOG_ALLOCATIONS

static MAllocator* sAllocator;

#ifndef M_CLIB_DISABLE

void* default_malloc(void* alloc, size_t size) {
    return malloc(size);
}

void* default_realloc(void* alloc, void* mem, size_t oldSize, size_t newSize) {
    return realloc(mem, newSize);
}

void default_free(void* alloc, void* mem, size_t size) {
    free(mem);
}

static MAllocator sClibMAllocator = {
        default_malloc,
        default_realloc,
        default_free
};

void MMemUseClibAllocator() {
    sAllocator = &sClibMAllocator;
}

#endif

void MMemAllocSet(MAllocator* allocator) {
    sAllocator = allocator;
}

#ifdef M_MEM_DEBUG

/////////////////////////////////////////////////////////
// Memory overwrite checking

#define SENTINEL_BEFORE 0x1000
#define SENTINEL_AFTER 0x1000

typedef struct {
    size_t size;
    u8* start;
    u8* mem;
    const char* file;
    int line;
} MMemAllocInfo;

typedef struct {
    MMemAllocInfo* allocSlots;
    u32* freeSlots;
    b32 sMemDebugInitialized;
    u32 totalAllocations;
    u32 totalAllocatedBytes;
    u32 curAllocatedBytes;
    u32 maxAllocatedBytes;
} MMemDebugContext;

static MMemDebugContext sMemDebugContext;

MINLINE void* MMemDebug_ArrayMaybeGrow(void* a, size_t elementSize) {
    size_t minNeeded;
    u32 capacity;
    size_t oldSize;
    MArrayHeader* p;
    if (a) {
        p = M_ArrayHeader(a);
        minNeeded = p->size + 1;
        if (p->capacity >= minNeeded) {
            return a;
        }
        capacity = p->capacity;
        oldSize = elementSize * capacity + sizeof(MArrayHeader);
    } else {
        p = NULL;
        minNeeded = 1;
        capacity = 0;
        oldSize = 0;
    }

    u32 newCapacity = (capacity > 4) ? (2 * capacity) : 8; // allocate for at least 8 elements
    if (minNeeded > newCapacity) {
        newCapacity = minNeeded;
    }

    size_t newSize = elementSize * newCapacity + sizeof(MArrayHeader);
    void* mem = sAllocator->reallocFunc(sAllocator, p, oldSize, newSize);
    if (mem == NULL)  {
        MLogf("MMemDebug_ArrayMaybeGrow realloc failed for %p", a);
        return NULL;
    }

    MArrayHeader* c = mem;
    void* b = (u8*)mem + sizeof(MArrayHeader);
    if (a == NULL) {
        c->size = 0;
    }
    c->capacity = newCapacity;
    return b;
}

#define MMemDebugArrayAddPtr(a) ((a) = MMemDebug_ArrayMaybeGrow(M_ArrayUnpack(a)), ((a) + M_ArrayHeader(a)->size++))

void MMemDebugInit() {
    if (!sMemDebugContext.sMemDebugInitialized) {
        sMemDebugContext.allocSlots = NULL;
        sMemDebugContext.freeSlots = NULL;
        sMemDebugContext.sMemDebugInitialized = TRUE;
    }
}

void MMemDebugDeinit(void) {
    if (!sMemDebugContext.sMemDebugInitialized) {
        return;
    }

    MLogf("Total allocations: %ld", sMemDebugContext.totalAllocations);
    MLogf("Total allocated: %ld bytes", sMemDebugContext.totalAllocatedBytes);
    MLogf("Max memory used: %ld bytes", sMemDebugContext.maxAllocatedBytes);

    MMemDebugCheckAll();

    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        if (memAlloc->start) {
            MLogf("Leaking allocation: %s:%d 0x%p (%d)", memAlloc->file, memAlloc->line, memAlloc->start,
                  memAlloc->size);
        }
    }

    if (sMemDebugContext.allocSlots) {
        sAllocator->freeFunc(sAllocator, M_ArrayHeader(sMemDebugContext.allocSlots),
                             M_ArrayHeader(sMemDebugContext.allocSlots)->capacity * sizeof(MMemAllocInfo));
        sMemDebugContext.allocSlots = NULL;
    }

    if (sMemDebugContext.freeSlots) {
        sAllocator->freeFunc(sAllocator, M_ArrayHeader(sMemDebugContext.freeSlots),
                             M_ArrayHeader(sMemDebugContext.freeSlots)->capacity * sizeof(u32));
        sMemDebugContext.freeSlots = NULL;
    }

    sMemDebugContext.sMemDebugInitialized = FALSE;
}

static u8 sMagicSentinelValues[4] = { 0x1d, 0xdf, 0x83, 0xc7 };

void MMemDebugSentinelSet(void* mem, size_t size) {
    u8* ptr = (u8*)mem;

    for (u32 i = 0; i < size; ++i) {
        ptr[i] = sMagicSentinelValues[i & 0x3];
    }
}

u32 MMemDebugSentinelCheck(void* mem, size_t size) {
    u8* ptr = (u8*)mem;
    u32 bytesOverwritten = 0;

    for (u32 i = 0; i < size; ++i) {
        if (ptr[i] != sMagicSentinelValues[i & 0x3]) {
            bytesOverwritten++;
        }
    }

    return bytesOverwritten;
}

static void LogBytesOverwritten(u8* startSentintel, u8* startOverwrite, u32 bytesOverwritten, b32 isAfter) {
    if (isAfter) {
        MLogf("offset: %d bytes: %d", startOverwrite - startSentintel, bytesOverwritten);
    } else {
        MLogf("offset: %d bytes: %d", startOverwrite - (SENTINEL_BEFORE + startSentintel), bytesOverwritten);
    }

    MLogBytes(startOverwrite, bytesOverwritten);
}

void MMemDebugSentinelCheckMLog(void* sentinelMemStart, size_t size, b32 isAfter) {
    u8* sentinelStart = (u8*)sentinelMemStart;
    u32 bytesOverwritten = 0;

    int state = 0;
    u8* overwriteStart = 0;
    for (u32 i = 0; i < size; ++i) {
        if (sentinelStart[i] != sMagicSentinelValues[i & 0x3]) {
            if (state == 0) {
                overwriteStart = sentinelStart + i;
                state = 1;
                bytesOverwritten = 0;
            }
            bytesOverwritten++;
        } else if (state == 1) {
            LogBytesOverwritten(sentinelStart, overwriteStart, bytesOverwritten, isAfter);
            state = 0;
        }
    }

    if (state == 1) {
        LogBytesOverwritten(sentinelStart, overwriteStart, bytesOverwritten, isAfter);
    }
}

b32 MMemDebugCheckMemAlloc(MMemAllocInfo* memAlloc) {
    if (!memAlloc->mem) {
        return FALSE;
    }

    u32 beforeBytes = MMemDebugSentinelCheck(memAlloc->mem - SENTINEL_BEFORE, SENTINEL_BEFORE);
    u32 afterBytes = MMemDebugSentinelCheck(memAlloc->mem + memAlloc->size, SENTINEL_AFTER);

    if (beforeBytes || afterBytes) {
        MLogf("MMemDebugCheck: %s:%d : 0x%p [%d]", memAlloc->file, memAlloc->line, memAlloc->mem,
              memAlloc->size);
    }

    if (beforeBytes) {
        MLogf("    %d bytes overwritten before:", beforeBytes);
        MMemDebugSentinelCheckMLog(memAlloc->mem - SENTINEL_BEFORE, SENTINEL_BEFORE, FALSE);
        MMemDebugSentinelSet(memAlloc->mem - SENTINEL_BEFORE, SENTINEL_BEFORE);
    }

    if (afterBytes) {
        MLogf("    %d bytes overwritten after:", afterBytes);
        MMemDebugSentinelCheckMLog(memAlloc->mem + memAlloc->size, SENTINEL_AFTER, TRUE);
        MLogBytes(memAlloc->mem + SENTINEL_BEFORE, 20);
        MMemDebugSentinelSet(memAlloc->mem + memAlloc->size, SENTINEL_AFTER);
    }

    return !(afterBytes | beforeBytes);
}

b32 MMemDebugCheck(void* p) {
    MMemAllocInfo* memAllocFound = NULL;
    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        if (p == memAlloc->mem) {
            memAllocFound = memAlloc;
            break;
        }
    }

    if (memAllocFound == NULL) {
        MLogf("MMemDebugCheck called on invalid ptr %p", p);
        return FALSE;
    }

    return MMemDebugCheckMemAlloc(memAllocFound);
}

b32 MMemDebugCheckAll() {
    b32 memOk = TRUE;
    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        if (!MMemDebugCheckMemAlloc(memAlloc)) {
            memOk = FALSE;
        }
    }

    return memOk;
}

b32 MMemDebugListAll() {
    b32 memOk = TRUE;
    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        MLogf("%s:%d %p %d", memAlloc->file, memAlloc->line, memAlloc->mem, memAlloc->size);
        if (!MMemDebugCheckMemAlloc(memAlloc)) {
            memOk = FALSE;
        }
    }

    return memOk;
}

#endif

static void* M_MallocInternal(MDEBUG_SOURCE_DEFINE size_t size) {
#ifdef M_MEM_DEBUG
    if (!sMemDebugContext.sMemDebugInitialized) {
        MMemDebugInit();
#ifndef M_CLIB_DISABLE
        atexit(MMemDebugDeinit);
#endif
    }

    MMemAllocInfo* memAlloc = NULL;
    int pos = 0;
    if (MArraySize(sMemDebugContext.freeSlots)) {
        pos = MArrayPop(sMemDebugContext.freeSlots);
        memAlloc = sMemDebugContext.allocSlots + pos;
    } else {
        memAlloc = MMemDebugArrayAddPtr(sMemDebugContext.allocSlots);
        pos = MArraySize(sMemDebugContext.allocSlots) - 1;
    }

    size_t start = SENTINEL_BEFORE;
    size_t allocSize = start + size + SENTINEL_AFTER;
    memAlloc->size = size;
    memAlloc->start = (u8*)sAllocator->mallocFunc(sAllocator, allocSize);
    memAlloc->mem = (u8*)memAlloc->start + SENTINEL_BEFORE;
    memAlloc->file = file;
    memAlloc->line = line;

    sMemDebugContext.totalAllocations += 1;
    sMemDebugContext.totalAllocatedBytes += size;
    sMemDebugContext.curAllocatedBytes += size;
    if (sMemDebugContext.curAllocatedBytes > sMemDebugContext.maxAllocatedBytes) {
        sMemDebugContext.maxAllocatedBytes = sMemDebugContext.curAllocatedBytes;
    }

    MMemDebugSentinelSet(memAlloc->mem - SENTINEL_BEFORE, SENTINEL_BEFORE);
    MMemDebugSentinelSet(memAlloc->mem + memAlloc->size, SENTINEL_AFTER);

#ifdef M_LOG_ALLOCATIONS
    MLogf("-> 0x%p", memAlloc->mem);
#endif
    return memAlloc->mem;
#endif
}

void* M_Malloc(MDEBUG_SOURCE_DEFINE size_t size) {
#ifndef M_CLIB_DISABLE
    if (!sAllocator) {
        sAllocator = &sClibMAllocator;
    }
#endif

#ifdef M_MEM_DEBUG
#ifdef M_LOG_ALLOCATIONS
#ifdef M_BACKTRACE
    MLogf("malloc(%d):", size);
    MLogStacktrace();
#else
    MLogf("malloc(%d) %s:%d", size, file, line);
#endif
#endif
    return M_MallocInternal(MDEBUG_SOURCE_PASS size);
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("malloc %d", size);
    void* mem = sAllocator->mallocFunc(sAllocator, size);
    MLogf("-> 0x%p", mem);
    return mem;
#else
    return sAllocator->mallocFunc(sAllocator, size);
#endif
#endif
}

void* M_MallocZ(MDEBUG_SOURCE_DEFINE size_t size) {
    void* r = M_Malloc(MDEBUG_SOURCE_PASS size);
    if (r) {
        memset(r, 0, size);
    }
    return r;
}

void M_Free(MDEBUG_SOURCE_DEFINE void* p, size_t size) {
#ifndef M_CLIB_DISABLE
    if (!sAllocator) {
        sAllocator = &sClibMAllocator;
    }
#endif
    if (p == NULL) {
        return;
    }

#ifdef M_MEM_DEBUG
    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        if (p == memAlloc->mem) {
            MMemDebugCheckMemAlloc(memAlloc);
            if (size != memAlloc->size) {
#ifdef M_BACKTRACE
                MLogf("MFree 0x%p called with wrong size: %d - should be: %d", p, size, memAlloc->size);
                MLogStacktrace();
#else
                MLogf("MFree %s:%d 0x%p called with wrong size: %d - should be: %d", file, line, p, size,
                      memAlloc->size);
#endif
            }
            sAllocator->freeFunc(sAllocator, memAlloc->start, SENTINEL_BEFORE + memAlloc->size + SENTINEL_AFTER);
            sMemDebugContext.curAllocatedBytes -= memAlloc->size;
            memAlloc->start = NULL;
            memAlloc->mem = NULL;
#ifdef M_LOG_ALLOCATIONS
#ifdef M_BACKTRACE
            MLogf("free(0x%p) size: %d [allocated @ %s:%d]", p, memAlloc->size, memAlloc->file, memAlloc->line);
            MLogStacktrace();
#else
            MLogf("free(0x%p) size: %d %s:%d [allocated @ %s:%d]", p, memAlloc->size, file, line, memAlloc->file, memAlloc->line);
#endif
#endif
            *(MMemDebugArrayAddPtr(sMemDebugContext.freeSlots)) = i;
            return;
        }
    }

    MLogf("MFree %s:%d called on invalid ptr: 0x%p", file, line, p);
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("free(0x%p)", p);
#endif
    sAllocator->freeFunc(sAllocator, p, size);
#endif
}

void* M_Realloc(MDEBUG_SOURCE_DEFINE void* p, size_t oldSize, size_t newSize) {
#ifndef M_CLIB_DISABLE
    if (!sAllocator) {
        sAllocator = &sClibMAllocator;
    }
#endif

#ifdef M_MEM_DEBUG
#ifdef M_LOG_ALLOCATIONS
#ifdef M_BACKTRACE
    MLogf("realloc(0x%p, %d, %d)", p, oldSize, newSize);
    MLogStacktrace();
#else
    MLogf("realloc(0x%p, %d, %d) [%s:%d]", p, oldSize, newSize, file, line);
#endif
#endif
    if (p == NULL) {
        return M_MallocInternal(MDEBUG_SOURCE_PASS newSize);
    }

    MMemAllocInfo* memAllocFound = NULL;
    for (u32 i = 0; i < MArraySize(sMemDebugContext.allocSlots); ++i) {
        MMemAllocInfo* memAlloc = sMemDebugContext.allocSlots + i;
        if (p == memAlloc->mem) {
            MMemDebugCheckMemAlloc(memAlloc);
            memAllocFound = memAlloc;
            break;
        }
    }

    if (memAllocFound == NULL) {
        MLogf("MRealloc called on invalid ptr at line: %s:%d %p", file, line, p);
        return p;
    }

    if (memAllocFound->size != oldSize) {
        MLogf("MRealloc %s:%d 0x%p called with old size: %d - should be: %d", file, line, p, oldSize,
              memAllocFound->size);
    }

    sMemDebugContext.totalAllocations += 1;
    sMemDebugContext.totalAllocatedBytes += newSize;
    sMemDebugContext.curAllocatedBytes += newSize - memAllocFound->size;
    if (sMemDebugContext.curAllocatedBytes > sMemDebugContext.maxAllocatedBytes) {
        sMemDebugContext.maxAllocatedBytes = sMemDebugContext.curAllocatedBytes;
    }

    size_t allocSize = SENTINEL_BEFORE + newSize + SENTINEL_AFTER;
    memAllocFound->size = newSize;
    memAllocFound->start = (u8*)sAllocator->reallocFunc(sAllocator, memAllocFound->start,
                                                        SENTINEL_BEFORE + oldSize + SENTINEL_AFTER, allocSize);
    memAllocFound->mem = memAllocFound->start + SENTINEL_BEFORE;
    memAllocFound->file = file;
    memAllocFound->line = line;

    MMemDebugSentinelSet( memAllocFound->start, SENTINEL_BEFORE);
    MMemDebugSentinelSet(memAllocFound->mem + memAllocFound->size, SENTINEL_AFTER);

#ifdef M_LOG_ALLOCATIONS
    MLogf("-> %p (resized)", memAllocFound->mem);
#endif
    return memAllocFound->mem;
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("realloc(0x%p, %d, %d)", p, oldSize, newSize);
    void* mem = sAllocator->reallocFunc(sAllocator, p, oldSize, newSize);
    MLogf("-> 0x%p (resized)", mem);
    return mem;
#else
    return sAllocator->reallocFunc(sAllocator, p, oldSize, newSize);
#endif
#endif
}

void* M_ReallocZ(MDEBUG_SOURCE_DEFINE void* p, size_t oldSize, size_t newSize) {
    void* r = M_Realloc(MDEBUG_SOURCE_PASS p, oldSize, newSize);
    if (r) {
        memset(r, 0, newSize);
    }
    return r;
}

static const char* sHexChars = "0123456789abcdef";
#define M_LOG_BYTES_PERLINE 64

void MLogBytes(const u8* mem, u32 len) {
    const int bytesPerLine = M_LOG_BYTES_PERLINE; // must be power of two - see AND (&) below
    char buff[M_LOG_BYTES_PERLINE + 1];
    int buffPos = 0;

    for (int j  = 0; j < len; ++j) {
        u8 val = mem[j];
        buff[buffPos++] = sHexChars[val >> 4];
        buff[buffPos++] = sHexChars[val & 0xf];
        if ((buffPos & (bytesPerLine - 1)) == 0) {
            buff[buffPos] = 0;
            MLog(buff);
            buffPos = 0;
        }
    }

    if (buffPos) {
        buff[buffPos] = 0;
        MLog(buff);
    }
}

/////////////////////////////////////////////////////////
// Dynamic Array

void* M_ArrayInit(MDEBUG_SOURCE_DEFINE void* a, size_t elementSize, size_t minNeeded) {
    if (a) {
        MArrayHeader* p = M_ArrayHeader(a);
        p->size = 0;
        return M_ArrayGrow(MDEBUG_SOURCE_PASS a, p, elementSize, minNeeded);
    } else {
        u32 capacity = 8; // allocate for at least 8 elements
        if (capacity < minNeeded) {
            capacity = minNeeded;
        }
        size_t newSize =  elementSize * capacity + sizeof(MArrayHeader);
        void* mem = M_Malloc(MDEBUG_SOURCE_PASS newSize);
        if (mem == NULL)  {
            MLogf("M_ArrayInit malloc returned NULL");
            return NULL;
        }

        MArrayHeader* p = mem;
        void* b = (u8*)mem + sizeof(MArrayHeader);
        if (a == NULL) {
            p->size = 0;
        }
        p->capacity = capacity;
        return b;
    }
}

void* M_ArrayGrow(MDEBUG_SOURCE_DEFINE void* a, MArrayHeader* p, size_t elementSize, size_t minNeeded) {
    u32 capacity;
    size_t oldSize;
    if (p) {
        capacity = p->capacity;
        oldSize = elementSize * capacity + sizeof(MArrayHeader);
    } else {
        capacity = 0;
        oldSize = 0;
    }

    if (a && minNeeded <= capacity) {
        return a;
    }

    // allocate for at least 8 elements
    u32 newCapacity = (capacity > 4) ? (2 * capacity) : 8;
    if (minNeeded > newCapacity) {
        newCapacity = minNeeded;
    }

    size_t newSize =  elementSize * newCapacity + sizeof(MArrayHeader);
    void* mem = M_Realloc(MDEBUG_SOURCE_PASS p, oldSize, newSize);
    if (mem == NULL)  {
        MLogf("M_ArrayGrow realloc failed for %p", a);
        return 0;
    }

    MArrayHeader* c = mem;
    void* b = (u8*)mem + sizeof(MArrayHeader);
    if (a == NULL) {
        c->size = 0;
    }
    c->capacity = newCapacity;
    return b;
}

void M_ArrayFree(MDEBUG_SOURCE_DEFINE void* a, size_t itemSize) {
    MArrayHeader* p = M_ArrayHeader(a);
    M_Free(MDEBUG_SOURCE_PASS p, itemSize * p->capacity + sizeof(MArrayHeader));
}

/////////////////////////////////////////////////////////
// Mem buffer reading / writing

void M_MemResize(MMemIO* memIO, u32 newMinSize) {
    u32 newSize = newMinSize;
    if (memIO->capacity * 2 > newSize) {
        newSize = memIO->capacity * 2;
    }
    memIO->mem = MRealloc(memIO->mem, memIO->capacity, newSize);
    memIO->pos = memIO->mem + memIO->size;
    memIO->capacity = newSize;
}

void MMemInitAlloc(MMemIO* memIO, u32 size) {
    u8* mem = (u8*)MMalloc(size);
    MMemInit(memIO, mem, size);
}

void MMemInit(MMemIO* memIO, u8* mem, u32 size) {
    memIO->pos = mem;
    memIO->mem = mem;
    memIO->capacity = size;
    memIO->size = 0;
}

void MMemReadInit(MMemIO* memIO, u8* mem, u32 size) {
    memIO->pos = mem;
    memIO->mem = mem;
    memIO->capacity = size;
    memIO->size = size;
}

void MMemGrowBytes(MMemIO* memIO, u32 growBy) {
    i32 newSize = memIO->size + growBy;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }
}

u8* MMemAddBytes(MMemIO* memIO, u32 growBy) {
    i32 newSize = memIO->size + growBy;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8* start = memIO->pos;
    memIO->pos += growBy;
    memIO->size = newSize;
    return start;
}

u8* MMemAddBytesZero(MMemIO* memIO, u32 size) {
    u32 newSize = memIO->size + size;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8* start = memIO->pos;
    memset(memIO->pos, 0, size);
    memIO->pos += size;
    memIO->size = newSize;
    return start;
}

void MMemWriteI8(MMemIO* memIO, i8 val) {
    u32 newSize = memIO->size + 1;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    *(memIO->pos) = val;

    memIO->pos += 1;
    memIO->size = newSize;
}

void MMemWriteU8(MMemIO* memIO, u8 val) {
    u32 newSize = memIO->size + 1;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    *(memIO->pos) = val;

    memIO->pos += 1;
    memIO->size = newSize;
}

void MMemWriteU16BE(MMemIO* memIO, u16 val) {
    u32 newSize = memIO->size + 2;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val >> 8, val & 0xff };
    memmove(memIO->pos, a, 2);
    memIO->pos += 2;
    memIO->size = newSize;
}

void MMemWriteU16LE(MMemIO* memIO, u16 val) {
    u32 newSize = memIO->size + 2;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val & 0xff, val >> 8 };
    memmove(memIO->pos, a, 2);
    memIO->pos += 2;
    memIO->size = newSize;
}

void MMemWriteI16BE(MMemIO* memIO, i16 val) {
    u32 newSize = memIO->size + 2;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val >> 8, val & 0xff };
    memmove(memIO->pos, a, 2);
    memIO->pos += 2;
    memIO->size = newSize;
}

void MMemWriteI16LE(MMemIO* memIO, i16 val) {
    u32 newSize = memIO->size + 2;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val & 0xff, val >> 8 };
    memmove(memIO->pos, a, 2);
    memIO->pos += 2;
    memIO->size = newSize;
}

void MMemWriteU32BE(MMemIO* memIO, u32 val) {
    u32 newSize = memIO->size + 4;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val >> 24, (val >> 16) & 0xff, (val >> 8) & 0xff, (val & 0xff) };
    memmove(memIO->pos, a, 4);
    memIO->pos += 4;
    memIO->size = newSize;
}

void MMemWriteU32LE(MMemIO* memIO, u32 val) {
    u32 newSize = memIO->size + 4;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = { (val & 0xff), (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) };
    memmove(memIO->pos, a, 4);
    memIO->pos += 4;
    memIO->size = newSize;
}

void MMemWriteI32BE(MMemIO* memIO, i32 val) {
    u32 newSize = memIO->size + 4;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = {val >> 24, (val >> 16) & 0xff, (val >> 8) & 0xff, (val & 0xff) };
    memmove(memIO->pos, a, 4);
    memIO->pos += 4;
    memIO->size = newSize;
}

void MMemWriteI32LE(MMemIO* memIO, i32 val) {
    u32 newSize = memIO->size + 4;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    u8 a[] = { (val & 0xff), (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) };
    memmove(memIO->pos, a, 4);
    memIO->pos += 4;
    memIO->size = newSize;
}

void MMemWriteU8CopyN(MMemIO*restrict memIO, u8*restrict src, u32 size) {
    if (size == 0) {
        return;
    }

    u32 newSize = memIO->size + size;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    memmove(memIO->pos, src, size);

    memIO->pos += size;
    memIO->size = newSize;
}

void MMemWriteI8CopyN(MMemIO*restrict memIO, i8*restrict src, u32 size) {
    if (size == 0) {
        return;
    }

    u32 newSize = memIO->size + size;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }

    memmove(memIO->pos, src, size);

    memIO->pos += size;
    memIO->size = newSize;
}

i32 MMemReadI8(MMemIO*restrict memIO, i8*restrict val) {
    if (memIO->pos >= memIO->mem + memIO->size) {
        return -1;
    }

    *(val) = *((i8*)(memIO->pos++));

    return 0;
}

i32 MMemReadU8(MMemIO*restrict memIO, u8*restrict val) {
    if (memIO->pos >= memIO->mem + memIO->size) {
        return -1;
    }

    *(val) = *(memIO->pos++);

    return 0;
}

i32 MMemReadI16(MMemIO*restrict memIO, i16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 2);
    memIO->pos += 2;
    return 0;
}

i32 MMemReadI16BE(MMemIO*restrict memIO, i16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i16 v;
    memmove(&v, memIO->pos, 2);
    *(val) = MBIGENDIAN16(v);
#else
    memmove(val, memIO->pos, 2);
#endif
    memIO->pos += 2;
    return 0;
}

i32 MMemReadI16LE(MMemIO*restrict memIO, i16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    i16 v;
    memmove(&v, memIO->pos, 2);
    *(val) = MLITTLEENDIAN16(v);
#else
    memmove(val, memIO->pos, 2);
#endif
    memIO->pos += 2;
    return 0;
}

i32 MMemReadU16(MMemIO*restrict memIO, u16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 2);
    memIO->pos += 2;
    return 0;
}

i32 MMemReadU16BE(MMemIO*restrict memIO, u16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    u16 v;
    memmove(&v, memIO->pos, 2);
    *(val) = MBIGENDIAN16(v);
#else
    memmove(val, memIO->pos, 2);
#endif
    memIO->pos += 2;
    return 0;
}

i32 MMemReadU16LE(MMemIO*restrict memIO, u16*restrict val) {
    if (memIO->pos + 2 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    u16 v;
    memmove(&v, memIO->pos, 2);
    *(val) = MLITTLEENDIAN16(v);
#else
    memmove(val, memIO->pos, 2);
#endif
    memIO->pos += 2;
    return 0;
}

i32 MMemReadI32(MMemIO*restrict memIO, i32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 4);
    memIO->pos += 4;
    return 0;
}

i32 MMemReadI32LE(MMemIO*restrict memIO, i32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->pos, 4);
    *(val) = MLITTLEENDIAN32(v);
#else
    memmove(val, memIO->pos, 4);
#endif
    memIO->pos += 4;
    return 0;
}

i32 MMemReadI32BE(MMemIO*restrict memIO, i32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i32 v;
    memmove(&v, memIO->pos, 4);
    *(val) = MBIGENDIAN32(v);
#else
    memmove(val, memIO->pos, 4);
#endif
    memIO->pos += 4;
    return 0;
}

i32 MMemReadU32(MMemIO*restrict memIO, u32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 4);
    memIO->pos += 4;
    return 0;
}

i32 MMemReadU32LE(MMemIO*restrict memIO, u32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->pos, 4);
    *(val) = MLITTLEENDIAN32(v);
#else
    memmove(val, memIO->pos, 4);
#endif
    memIO->pos += 4;
    return 0;
}

i32 MMemReadU32BE(MMemIO*restrict memIO, u32*restrict val) {
    if (memIO->pos + 4 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i32 v;
    memmove(&v, memIO->pos, 4);
    *(val) = MBIGENDIAN32(v);
#else
    memmove(val, memIO->pos, 4);
#endif
    memIO->pos += 4;
    return 0;
}

i32 MMemReadI64(MMemIO* memIO, i64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 8);

    memIO->pos += 8;
    return 0;
}

i32 MMemReadI64BE(MMemIO* memIO, i64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i64 v;
    memmove(&v, memIO->pos, 8);
    *(val) = MBIGENDIAN64(v);
#else
    memmove(val, memIO->pos, 8);
#endif
    memIO->pos += 8;
    return 0;
}

i32 MMemReadI64LE(MMemIO* memIO, i64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->pos, 8);
    *(val) = MLITTLEENDIAN64(v);
#else
    memmove(val, memIO->pos, 8);
#endif
    memIO->pos += 8;
    return 0;
}

i32 MMemReadU64(MMemIO* memIO, u64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

    memmove(val, memIO->pos, 8);

    memIO->pos += 8;
    return 0;
}

i32 MMemReadU64BE(MMemIO* memIO, u64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    memmove(val, memIO->pos, 8);
#else
    u64 v;
    memmove(&v, memIO->pos, 8);
    *(val) = MBIGENDIAN64(v);
#endif
    memIO->pos += 8;
    return 0;
}

i32 MMemReadU64LE(MMemIO* memIO, u64* val) {
    if (memIO->pos + 8 > memIO->mem + memIO->size) {
        return -1;
    }

#ifdef MBIGENDIAN
    i64 v;
    memmove(&v, memIO->pos, 8);
    *(val) = MLITTLEENDIAN64(v);
#else
    memmove(val, memIO->pos, 8);
#endif
    memIO->pos += 8;
    return 0;
}

i32 MMemReadU8CopyN(MMemIO* memIO, u8* dest, u32 size) {
    if (memIO->pos + size > memIO->mem + memIO->size) {
        return -1;
    }
    memmove(dest, memIO->pos, size);
    memIO->pos += size;
    return 0;
}

i32 MMemReadCharCopyN(MMemIO* memIO, char* dest, u32 size) {
    if (memIO->pos + size > memIO->mem + memIO->size) {
        return -1;
    }
    memmove(dest, memIO->pos, size);
    memIO->pos += size;
    return 0;
}

i32 MMemReadCopy(MMemIO* reader, MMemIO* out, u32 size) {
    MMemGrowBytes(out, size);
    i32 r = MMemReadU8CopyN(reader, out->pos, size);
    if (r == 0) {
        out->size += size;
        out->pos += size;
    }
    return r;
}

char* MMemReadStr(MMemIO* memIO) {
    u8* oldPos = memIO->pos;
    while (*memIO->pos) { memIO->pos++; }
    return (char*)oldPos;
}


/////////////////////////////////////////////////////////
// String parsing / conversion functions

int M_IsWhitespace(char c) {
    return (c == '\n' || c == '\r' || c == '\t' || c == ' ');
}

int M_IsSpaceTab(char c) {
    return (c == '\t' || c == ' ');
}

int M_IsNewLine(char c) {
    return (c == '\n' || c == '\r');
}

const char* MStrEnd(const char* str) {
    if (!str) {
        return NULL;
    }

    while (*str) {
        str++;
    }

    return str;
}

i32 MParseI32(const char* start, const char* end, i32* out) {
    int val = 0;
    int begin = 1;
    int base = 10;

    for (const char* pos = start; pos < end; pos++) {
        char c = *pos;
        if (begin && M_IsWhitespace(c)) {
            continue;
        }

        begin = 0;

        int p = 0;
        if (c >= '0' && c <= '9') {
            p = c - '0';
        } else {
            return MParse_NOT_A_NUMBER;
        }

        val = (val * base) + p;
    }

    *out = val;

    return MParse_SUCCESS;
}

i32 MParseI32Hex(const char* start, const char* end, i32* out) {
    int val = 0;
    int begin = 1;
    int base = 16;

    for (const char* pos = start; pos < end; pos++) {
        char c = *pos;
        if (begin && M_IsWhitespace(c)) {
            continue;
        }

        begin = 0;

        int p = 0;
        if (c >= '0' && c <= '9') {
            p = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            p = 10 + c - 'a';
        } else if (c >= 'A' && c <= 'F') {
            p = 10 + c - 'A';
        } else {
            return MParse_NOT_A_NUMBER;
        }

        val = (val * base) + p;
    }

    *out = val;

    return MParse_SUCCESS;
}

i32 MStrCmp(const char* str1, const char* str2) {
    char c1 = *str1++;
    char c2 = *str2++;
    while (TRUE) {
        if (c1 == 0) {
            if (c2 == 0) {
                return 0;
            } else {
                return -1;
            }
        } else if (c2 == 0) {
            return -1;
        }

        if (c1 != c2) {
            if (c1 > c2) {
                return 1;
            } else {
                return -1;
            }
        }

        c1 = *str1++;
        c2 = *str2++;
    }
}

i32 MStrCmp3(const char* str1, const char* str2Start, const char* str2End) {
    char c1 = *str1, c2;
    for (; str2Start < str2End && c1 != 0; c1 = *str1) {
        c2 = *str2Start;
        if (c1 != c2) {
            if (c1 > c2) {
                return 1;
            } else {
                return -1;
            }
        }
        str1++;
        str2Start++;
    }

    if (c1 == 0 && str2Start == str2End) {
        return 0;
    }

    if (c1 == 0) {
        return -1;
    } else {
        return 1;
    }
}

void MStrU32ToBinary(u32 val, int size, char* out) {
    out[size] = '\0';

    u32 z = (1 << (size - 1));
    for (int i = 0; i < size; i++, z >>= 1) {
        char c = ((val & z) == z) ? '1' : '0';
        out[i] = c;
    }
}

i32 MStringAppend(MMemIO* memIo, const char* str) {
    i32 len = MStrLen(str);

    i32 newSize = memIo->size + len + 1;
    if (newSize > memIo->capacity) {
        M_MemResize(memIo, newSize);
    }

    memcpy(memIo->pos, str, len + 1);
    memIo->size += len;
    memIo->pos += len;

    return len;
}

i32 MFileWriteDataFully(const char* filePath, u8* data, u32 size) {
    MFile file = MFileWriteOpen(filePath);
    if (file.open) {
        i32 r = MFileWriteData(&file, data, size);
        MFileClose(&file);
        return r;
    }
    return -1;
}

#if defined(M_BACKTRACE)

#if defined(M_LIBBACKTRACE)
#include <backtrace.h>
#include <stdio.h>

struct backtrace_state* sBacktraceState;

static int MLogStacktrace_PrintBacktraceCallback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
    (void)data; // Avoid unused variable warning
    if (function) {
        MLogf("  %s() %s:%d", function, filename, lineno);
    } else if (filename != NULL && lineno != 0) {
        MLogf("  %?%?%?() %s:%d", filename, lineno);
    }
    return 0;
}

static void MLogStacktrace_ErrorCallback(void *data, const char *msg, int errnum) {
    (void)data;
    MLogf("   Backtrace error: %s (%d)", msg, errnum);
}

void MLogStacktrace() {
    if (!sBacktraceState) {
        sBacktraceState = backtrace_create_state(NULL, 1, MLogStacktrace_ErrorCallback, NULL);
        if (sBacktraceState == NULL) {
            MLog("Failed to create backtrace state");
            return;
        }
    }
    backtrace_full(sBacktraceState, 1, MLogStacktrace_PrintBacktraceCallback, MLogStacktrace_ErrorCallback, NULL);
}

typedef struct MStacktraceLine {
    char name[248];
    char file[256];
    int lineno;
    uintptr_t pc;
} MStacktraceLine;

typedef struct MStacktrace {
    MStacktraceLine lines[100]; // [100]
    int lineCount;
    u32 hash;
} MStacktrace;

// TODO: Add location hash to every allocation.
// TODO: maintain a map of pc & full pc stacktrace hashes to stacktraces
// TODO: Garbage collection cleanup function called to make room for new hashes
// TODO: Print stacktrace for allocation when reporting errors for stacktrace
static int GetStacktrace_PrintBacktraceCallback(void* data, uintptr_t pc, const char* filename, int lineno, const char* function) {
    MStacktrace* stacktrace = (MStacktrace*)data;
    MStacktraceLine* stacktraceLine = &(stacktrace->lines[stacktrace->lineCount]);
    if (function) {
        stacktraceLine->lineno = lineno;
        strcpy(stacktraceLine->name, function);
        strcpy(stacktraceLine->file, filename);
        stacktraceLine->pc = pc;
        stacktrace->lineCount++;
    } else if (filename != NULL && lineno != 0) {
        stacktraceLine->lineno = lineno;
        strcpy("???", function);
        strcpy(stacktraceLine->file, filename);
        stacktraceLine->pc = pc;
        stacktrace->lineCount++;
    }
    return 0;
}

static void GetStacktrace_ErrorCallback(void* data, const char* msg, int errnum) {
    (void)data;
    MLogf("MGetStacktrace error: %s (%d)", msg, errnum);
}

void MGetStacktrace(MStacktrace* stacktrace) {
    if (!sBacktraceState) {
        sBacktraceState = backtrace_create_state(NULL, 1, MLogStacktrace_ErrorCallback, NULL);
        if (sBacktraceState == NULL) {
            MLog("Failed to create backtrace state");
            return;
        }
    }
    backtrace_full(sBacktraceState, 1, GetStacktrace_PrintBacktraceCallback, GetStacktrace_ErrorCallback, stacktrace);
}

static int GetStacktraceHash_SimpleCallback(void* data, uintptr_t pc) {
    u32 hash = (*(u32*)data);
    hash = hash * 31 + pc;
    (*(u32*)data) = hash;
    return 0;
}

static void GetStacktraceHash_ErrorCallback(void* data, const char* msg, int errnum) {
    (void)data;
    MLogf("MGetStacktrace error: %s (%d)", msg, errnum);
}

typedef struct MStacktraceHash {
    u64 hash; // Hash of full stacktrace
    u64 pc; // Head PC of stacktrace
} MStacktraceHash;

u32 MGetStacktraceHash() {
    u32 hash = 0;
    if (!sBacktraceState) {
        sBacktraceState = backtrace_create_state(NULL, 1, MLogStacktrace_ErrorCallback, NULL);
        if (sBacktraceState == NULL) {
            MLog("Failed to create backtrace state");
            return 0;
        }
    }
    backtrace_simple(sBacktraceState, 1, GetStacktraceHash_SimpleCallback, GetStacktraceHash_ErrorCallback, &hash);
    return hash;
}

#elif defined(WIN32)

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

void MLogStacktrace() {
    // This will only work if you generate PDB files, MinGW GCC will only generate
    // DWARF debug info.
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    CONTEXT context = {};
    RtlCaptureContext(&context);

    STACKFRAME64 stackFrame = {};
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    int skipFrames = 1;

    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(),
                       &stackFrame, &context, NULL, SymFunctionTableAccess64,
                       SymGetModuleBase64, NULL)) {
        if (skipFrames) {
            skipFrames--;
            continue;
        }
        char buffer[sizeof(SYMBOL_INFO) + 256];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 255;
        DWORD64 address = stackFrame.AddrPC.Offset;
        DWORD64 displacement64;
        char* funcName = NULL;
        char* fileName = NULL;
        int lineNo = 0;

        if (SymFromAddr(process, address, &displacement64, symbol)) {
            funcName = symbol->Name;
        }

        // Source file and line number
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement32;
        if (SymGetLineFromAddr64(process, address, &displacement32, &line)) {
            fileName = line.FileName;
            lineNo = line.LineNumber;
        }

        if (funcName == NULL) {
            funcName = "???";
        }

        if (fileName != NULL && lineNo != 0) {
            MLogf("  %s() %s:%d", funcName, fileName, lineNo);
        } else {
            MLogf("  %s()", funcName);
        }

        if (MStrCmp("main", funcName) == 0) {
            break;
        }
    }

    SymCleanup(process);
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

void MLogStacktrace() {
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++) {
        MLogf("  %s()", strings[i]);
    }

    free(strings);
}
#endif

#endif
