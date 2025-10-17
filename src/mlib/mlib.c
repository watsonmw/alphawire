#include "mlib.h"

#ifndef M_CLIB_DISABLE
#include <stdlib.h>
#endif

#ifdef M_THREADING
#if defined(M_PTHREADS)
#include <pthread.h>

void MMutexLock(MMutex* m)   { pthread_mutex_lock((pthread_mutex_t*)m); }
void MMutexUnlock(MMutex* m) { pthread_mutex_unlock((pthread_mutex_t*)m); }

void MExecuteOnce(MOnce* once, void (*fn)(void)) {
    pthread_once((pthread_once_t*)once, fn);
}
#elif defined(_WIN32)
#include <windows.h>

void MMutexLock(MMutex* m)   { AcquireSRWLockExclusive((PSRWLOCK)m); }
void MMutexUnlock(MMutex* m) { ReleaseSRWLockExclusive((PSRWLOCK)m); }

MINTERNAL BOOL CALLBACK M_once_cb(PINIT_ONCE once, PVOID param, PVOID* ctx) {
    void (*fn)(void) = (void(*)(void))param; fn(); return TRUE;
}
void MExecuteOnce(MOnce* once, void (*fn)(void)) {
    InitOnceExecuteOnce((PINIT_ONCE)once, M_once_cb, (PVOID)fn, NULL);
}
#endif
#else
void MExecuteOnce(MOnce* once, void (*fn)(void)) {
    if (!*once) {
        fn();
        *once = TRUE;
    }
}
#endif

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

void MAllocatorMakeClibHeap(MAllocator* allocator) {
    allocator->mallocFunc = default_malloc;
    allocator->reallocFunc = default_realloc;
    allocator->freeFunc = default_free;
}

#endif

#ifdef M_MEM_DEBUG

/////////////////////////////////////////////////////////
// Memory overwrite checking

#define M_SENTINEL_BEFORE 0x1000
#define M_SENTINEL_AFTER 0x1000

MINLINE void* MMemDebug_ArrayMaybeGrow(MAllocator* allocator, void* a, size_t elementSize) {
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
    void* mem = allocator->reallocFunc(allocator, p, oldSize, newSize);
    if (mem == NULL)  {
        MBreakpointf("MMemDebug_ArrayMaybeGrow realloc failed for %p", a);
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

#define MMemDebugArrayAddPtr(allocator, a) ((a) = MMemDebug_ArrayMaybeGrow(allocator, M_ArrayUnpack(a)), ((a) + M_ArrayHeader(a)->size++))

#ifdef M_STACKTRACE
static void MMemDebug_AddStacktrace(MAllocator* alloc, MMemAllocInfo* memAlloc, int skipFrames) {
    // TODO: More efficient stacktrace lookup and storage
    MStacktrace currentStacktrace = {};
    MGetStacktrace(&currentStacktrace, skipFrames);
    memAlloc->stacktraceHash = currentStacktrace.hash;
    b32 found = FALSE;
    MArrayEachPtr(alloc->debug.stacktraces, it) {
        MStacktrace* stacktrace = (it.p);
        if (stacktrace->hash == currentStacktrace.hash) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        MStacktrace* stacktrace = MMemDebugArrayAddPtr(alloc, alloc->debug.stacktraces);
        memset(stacktrace, 0, sizeof(MStacktrace));
        MGetStacktrace(stacktrace, skipFrames);
    }
}

static MStacktrace* MMemDebug_FindStacktrace(MAllocator* alloc, MMemAllocInfo* memAlloc) {
    MArrayEachPtr(alloc->debug.stacktraces, it) {
        MStacktrace* stacktrace = (it.p);
        if (stacktrace->hash == memAlloc->stacktraceHash) {
            return stacktrace;
        }
    }
    return NULL;
}

static MStacktrace* MMemDebug_LogAllocStacktrace(MAllocator* alloc, MMemAllocInfo* memAlloc) {
    MStacktrace* stacktrace = MMemDebug_FindStacktrace(alloc, memAlloc);
    if (stacktrace) {
        MLogStacktrace(stacktrace);
    }
    return stacktrace;
}
#endif

void MMemDebugInit(MAllocator* allocator) {
    if (!allocator->debug.initialized) {
        allocator->debug.allocSlots = NULL;
        allocator->debug.freeSlots = NULL;
        allocator->debug.initialized = TRUE;
        allocator->debug.leakTracking = TRUE;
        allocator->debug.sentinelCheck = TRUE;
        allocator->debug.maxAllocatedBytes = 0;
        allocator->debug.curAllocatedBytes = 0;
        allocator->debug.totalAllocations = 0;
        allocator->debug.totalAllocatedBytes = 0;
    }
}

void MMemDebugDeinit(MAllocator* alloc) {
    MMemDebugDeinit2(alloc, TRUE);
}

void MMemDebugDeinit2(MAllocator* alloc, b32 logSummary) {
    if (!alloc->debug.initialized) {
        return;
    }

    if (logSummary) {
        const char* name = alloc->name;
        if (!name) {
            name = "default";
        }
        MLogf("Releasing Allocator '%s':", name);

        MLogf("   Total allocations: %d", alloc->debug.totalAllocations);
        MLogf("   Total allocated: %d bytes", alloc->debug.totalAllocatedBytes);
        MLogf("   Max memory used: %d bytes", alloc->debug.maxAllocatedBytes);
    }

    MMemDebugCheckAll(alloc);

    if (alloc->debug.leakTracking) {
        MArrayEachPtr(alloc->debug.allocSlots, it) {
            MMemAllocInfo* memAlloc = it.p;
            if (memAlloc->start) {
#ifdef M_STACKTRACE
                MBreakpointf("Leaking allocation: 0x%p (%zu bytes)", memAlloc->start, memAlloc->size);
                MMemDebug_LogAllocStacktrace(alloc, memAlloc);
#else
                MBreakpointf("Leaking allocation: %s:%d 0x%p (%d bytes)", memAlloc->file, memAlloc->line, memAlloc->start, memAlloc->size);
#endif
            }
        }
    }

    if (alloc->debug.allocSlots) {
        alloc->freeFunc(alloc, M_ArrayHeader(alloc->debug.allocSlots),
            M_ArrayHeader(alloc->debug.allocSlots)->capacity * sizeof(MMemAllocInfo));
        alloc->debug.allocSlots = NULL;
    }

    if (alloc->debug.freeSlots) {
        alloc->freeFunc(alloc, M_ArrayHeader(alloc->debug.freeSlots),
            M_ArrayHeader(alloc->debug.freeSlots)->capacity * sizeof(u32));
        alloc->debug.freeSlots = NULL;
    }

#ifdef M_STACKTRACE
    if (alloc->debug.stacktraces) {
        alloc->freeFunc(alloc, M_ArrayHeader(alloc->debug.stacktraces),
            M_ArrayHeader(alloc->debug.stacktraces)->capacity * sizeof(u32));
        alloc->debug.stacktraces = NULL;
    }
#endif

    alloc->debug.initialized = FALSE;
}

static u8 sMagicCanaryValues[4] = { 0x1d, 0xdf, 0x83, 0xc7 };

void MMemDebugCanarySet(void* mem, size_t size) {
    u8* ptr = (u8*)mem;

    for (u32 i = 0; i < size; ++i) {
        ptr[i] = sMagicCanaryValues[i & 0x3];
    }
}

u32 MMemDebugCanaryCheck(void* mem, size_t size) {
    u8* ptr = (u8*)mem;
    u32 bytesOverwritten = 0;

    for (u32 i = 0; i < size; ++i) {
        if (ptr[i] != sMagicCanaryValues[i & 0x3]) {
            bytesOverwritten++;
        }
    }

    return bytesOverwritten;
}

static void LogBytesOverwritten(u8* startSentintel, u8* startOverwrite, u32 bytesOverwritten, b32 isAfter) {
    if (isAfter) {
        MLogf("    [offset: %d bytes: %d]", startOverwrite - startSentintel, bytesOverwritten);
    } else {
        MLogf("    [offset: %d bytes: %d]", startOverwrite - (M_SENTINEL_BEFORE + startSentintel), bytesOverwritten);
    }

    MLogBytes(startOverwrite, bytesOverwritten);
}

void MMemDebugCanaryCheckMLog(void* sentinelMemStart, size_t size, b32 isAfter) {
    u8* sentinelStart = (u8*)sentinelMemStart;
    u32 bytesOverwritten = 0;

    int state = 0;
    u8* overwriteStart = NULL;
    for (u32 i = 0; i < size; ++i) {
        if (sentinelStart[i] != sMagicCanaryValues[i & 0x3]) {
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

b32 MMemDebugCheckMemAlloc(MAllocator* alloc, MMemAllocInfo* memAlloc) {
    if (!memAlloc->mem) {
        return FALSE;
    }

    if (!alloc->debug.sentinelCheck) {
        return TRUE;
    }

    u32 beforeBytes = MMemDebugCanaryCheck(memAlloc->mem - M_SENTINEL_BEFORE, M_SENTINEL_BEFORE);
    u32 afterBytes = MMemDebugCanaryCheck(memAlloc->mem + memAlloc->size, M_SENTINEL_AFTER);

    if (beforeBytes || afterBytes) {
#ifdef M_STACKTRACE
        MBreakpointf("MMemDebugCheck: 0x%p [%zu]", memAlloc->mem, memAlloc->size);
        MMemDebug_LogAllocStacktrace(alloc, memAlloc);
#else
        MBreakpointf("MMemDebugCheck: %s:%d : 0x%p [%d]", memAlloc->file, memAlloc->line, memAlloc->mem, memAlloc->size);
#endif
    }

    if (beforeBytes) {
        MLogf("  %d bytes overwritten before:", beforeBytes);
        MMemDebugCanaryCheckMLog(memAlloc->mem - M_SENTINEL_BEFORE, M_SENTINEL_BEFORE, FALSE);
        MMemDebugCanarySet(memAlloc->mem - M_SENTINEL_BEFORE, M_SENTINEL_BEFORE);
    }

    if (afterBytes) {
        MLogf("  %d bytes overwritten after:", afterBytes);
        MMemDebugCanaryCheckMLog(memAlloc->mem + memAlloc->size, M_SENTINEL_AFTER, TRUE);
        MMemDebugCanarySet(memAlloc->mem + memAlloc->size, M_SENTINEL_AFTER);
    }

    return !(afterBytes | beforeBytes);
}

b32 MMemDebugCheck(MAllocator* alloc, void* p) {
    MMemAllocInfo* memAllocFound = NULL;
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        if (p == memAlloc->mem) {
            memAllocFound = memAlloc;
            break;
        }
    }

    if (memAllocFound == NULL) {
        MBreakpointf("MMemDebugCheck called on invalid ptr %p", p);
#ifdef M_STACKTRACE
        MLogStacktraceCurrent(1);
#endif
        return FALSE;
    }

    return MMemDebugCheckMemAlloc(alloc, memAllocFound);
}

b32 MMemDebugCheckAll(MAllocator* alloc) {
    b32 memOk = TRUE;
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        if (memAlloc->mem && MMemDebugCheckMemAlloc(alloc, memAlloc)) {
            memOk = FALSE;
        }
    }

    return memOk;
}

b32 MMemDebugListAll(MAllocator* alloc) {
    b32 memOk = TRUE;
    MLogf("Allocations for %s:", alloc->name);
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        MLogf("   %s:%d %p %zu", memAlloc->file, memAlloc->line, memAlloc->mem, memAlloc->size);
        if (!MMemDebugCheckMemAlloc(alloc, memAlloc)) {
            memOk = FALSE;
        }
    }

    return memOk;
}

void MMemDebugFreePtrsInRange(MAllocator* alloc, u8* startAddress, u8* endAddress) {
    size_t compactedSize = 0;
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        if (memAlloc->mem >= startAddress && memAlloc->mem <= endAddress) {
            MMemDebugCheckMemAlloc(alloc, memAlloc);
            alloc->debug.curAllocatedBytes -= memAlloc->size;
            memAlloc->start = NULL;
            memAlloc->mem = NULL;
            memAlloc->size = 0;
        } else {
            // compact as we go
            alloc->debug.allocSlots[compactedSize] = *memAlloc;
            compactedSize++;
        }
    }

    MArrayResize(alloc, alloc->debug.allocSlots, compactedSize);
    MArrayResize(alloc, alloc->debug.freeSlots, 0);
}

static void* M_MallocDebug(MDEBUG_SOURCE_DEFINE MAllocator* alloc, size_t size) {
    if (!alloc->debug.initialized) {
        MMemDebugInit(alloc);
    }

    MMemAllocInfo* memAlloc = NULL;
    int pos = 0;
    if (MArraySize(alloc->debug.freeSlots)) {
        pos = MArrayPop(alloc->debug.freeSlots);
        memAlloc = alloc->debug.allocSlots + pos;
    } else {
        memAlloc = MMemDebugArrayAddPtr(alloc, alloc->debug.allocSlots);
        if (memAlloc == NULL) {
            return NULL;
        }
        pos = MArraySize(alloc->debug.allocSlots) - 1;
    }

    if (alloc->debug.sentinelCheck) {
        size_t start = M_SENTINEL_BEFORE;
        size_t allocSize = start + size + M_SENTINEL_AFTER;
        u8* mem = (u8*)alloc->mallocFunc(alloc, allocSize);
        if (mem == NULL) {
            return NULL;
        }
        memAlloc->start = mem;
        memAlloc->mem = (u8*)memAlloc->start + M_SENTINEL_BEFORE;
    } else {
        u8* mem = (u8*)alloc->mallocFunc(alloc, size);
        if (mem == NULL) {
            return NULL;
        }
        memAlloc->start = mem;
        memAlloc->mem = mem;
    }

    memAlloc->size = size;
    memAlloc->file = file;
    memAlloc->line = line;

#ifdef M_STACKTRACE
    MMemDebug_AddStacktrace(alloc, memAlloc, 2);
#endif

    alloc->debug.totalAllocations += 1;
    alloc->debug.totalAllocatedBytes += size;
    alloc->debug.curAllocatedBytes += size;
    if (alloc->debug.curAllocatedBytes > alloc->debug.maxAllocatedBytes) {
        alloc->debug.maxAllocatedBytes = alloc->debug.curAllocatedBytes;
    }

    if (alloc->debug.sentinelCheck) {
        MMemDebugCanarySet(memAlloc->mem - M_SENTINEL_BEFORE, M_SENTINEL_BEFORE);
        MMemDebugCanarySet(memAlloc->mem + memAlloc->size, M_SENTINEL_AFTER);
    }

#ifdef M_LOG_ALLOCATIONS
    MLogf("-> 0x%p", memAlloc->mem);
#endif
    return memAlloc->mem;
}

#endif

void* M_Malloc(MDEBUG_SOURCE_DEFINE MAllocator* alloc, size_t size) {
#ifdef M_MEM_DEBUG
#ifdef M_LOG_ALLOCATIONS
#ifdef M_STACKTRACE
    MLogf("malloc(%d):", size);
    MLogStacktraceCurrent(0);
#else
    MLogf("malloc(%d) %s:%d", size, file, line);
#endif
#endif
    return M_MallocDebug(MDEBUG_SOURCE_PASS alloc, size);
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("malloc %d", size);
    void* mem = alloc->mallocFunc(alloc, size);
    MLogf("-> 0x%p", mem);
    return mem;
#else
    return alloc->mallocFunc(alloc, size);
#endif
#endif
}

void M_Free(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* p, size_t size) {
    if (p == NULL) {
        return;
    }

#ifdef M_MEM_DEBUG
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        if (p == memAlloc->mem) {
            MMemDebugCheckMemAlloc(alloc, memAlloc);
            if (size != memAlloc->size) {
#ifdef M_STACKTRACE
                MBreakpointf("MFree 0x%p called with wrong size: %zu - should be: %zu", p, size, memAlloc->size);
                MLogStacktraceCurrent(0);
                MLog(" allocated @");
                MMemDebug_LogAllocStacktrace(alloc, memAlloc);
#else
                MBreakpointf("MFree %s:%d 0x%p called with wrong size: %zu - should be: %d", file, line, p, size,
                             memAlloc->size);
#endif
            }
            alloc->freeFunc(alloc, memAlloc->start, M_SENTINEL_BEFORE + memAlloc->size + M_SENTINEL_AFTER);
            alloc->debug.curAllocatedBytes -= memAlloc->size;
            memAlloc->start = NULL;
            memAlloc->mem = NULL;
#ifdef M_LOG_ALLOCATIONS
#ifdef M_STACKTRACE
            MLogf("free(0x%p) size: %zu [allocated @ %s:%d]", p, memAlloc->size, memAlloc->file, memAlloc->line);
            MLogStacktraceCurrent(1);
#else
            MLogf("free(0x%p) size: %zu %s:%d [allocated @ %s:%d]", p, memAlloc->size, file, line, memAlloc->file, memAlloc->line);
#endif
#endif
            *(MMemDebugArrayAddPtr(alloc, alloc->debug.freeSlots)) = it.i;
            return;
        }
    }

    MBreakpointf("MFree %s:%d called on invalid ptr: 0x%p", file, line, p);
#ifdef M_STACKTRACE
    MLogStacktraceCurrent(0);
#endif
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("free(0x%p)", p);
#endif
    alloc->freeFunc(alloc, p, size);
#endif
}

void* M_Realloc(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* p, size_t oldSize, size_t newSize) {
#ifdef M_MEM_DEBUG
#ifdef M_LOG_ALLOCATIONS
#ifdef M_STACKTRACE
    MLogf("realloc(0x%p, %zu, %zu)", p, oldSize, newSize);
    MLogStacktraceCurrent(0);
#else
    MLogf("realloc(0x%p, %zu, %zu) [%s:%d]", p, oldSize, newSize, file, line);
#endif
#endif
    if (p == NULL) {
        return M_MallocDebug(MDEBUG_SOURCE_PASS alloc, newSize);
    }

    MMemAllocInfo* memAllocFound = NULL;
    MArrayEachPtr(alloc->debug.allocSlots, it) {
        MMemAllocInfo* memAlloc = it.p;
        if (p == memAlloc->mem) {
            MMemDebugCheckMemAlloc(alloc, memAlloc);
            memAllocFound = memAlloc;
            break;
        }
    }

    if (memAllocFound == NULL) {
        MBreakpointf("MRealloc called on invalid ptr at line: %s:%d %p", file, line, p);
#ifdef M_STACKTRACE
        MLogStacktraceCurrent(0);
#endif
        return p;
    }

    if (memAllocFound->size != oldSize) {
        MBreakpointf("MRealloc %s:%d 0x%p called with old size: %zu - should be: %zu", file, line, p, oldSize,
              memAllocFound->size);
#ifdef M_STACKTRACE
        MMemDebug_LogAllocStacktrace(alloc, memAllocFound);
#endif
    }

    alloc->debug.totalAllocations += 1;
    alloc->debug.totalAllocatedBytes += newSize;
    alloc->debug.curAllocatedBytes += newSize - memAllocFound->size;
    if (alloc->debug.curAllocatedBytes > alloc->debug.maxAllocatedBytes) {
        alloc->debug.maxAllocatedBytes = alloc->debug.curAllocatedBytes;
    }

    if (alloc->debug.sentinelCheck) {
        size_t allocSize = M_SENTINEL_BEFORE + newSize + M_SENTINEL_AFTER;
        memAllocFound->size = newSize;
        memAllocFound->start = (u8*)alloc->reallocFunc(alloc, memAllocFound->start,
            M_SENTINEL_BEFORE + oldSize + M_SENTINEL_AFTER, allocSize);
        memAllocFound->mem = memAllocFound->start + M_SENTINEL_BEFORE;

        MMemDebugCanarySet(memAllocFound->start, M_SENTINEL_BEFORE);
        MMemDebugCanarySet(memAllocFound->mem + memAllocFound->size, M_SENTINEL_AFTER);
    } else {
        memAllocFound->size = newSize;
        memAllocFound->start = (u8*)alloc->reallocFunc(alloc, memAllocFound->start, oldSize, newSize);
        memAllocFound->mem = memAllocFound->start;
    }

    memAllocFound->file = file;
    memAllocFound->line = line;

#ifdef M_STACKTRACE
    MMemDebug_AddStacktrace(alloc, memAllocFound, 2);
#endif

#ifdef M_LOG_ALLOCATIONS
    MLogf("-> %p (resized)", memAllocFound->mem);
#endif
    return memAllocFound->mem;
#else
#ifdef M_LOG_ALLOCATIONS
    MLogf("realloc(0x%p, %d, %d)", p, oldSize, newSize);
    void* mem = smem->reallocFunc(alloc, p, oldSize, newSize);
    MLogf("-> 0x%p (resized)", mem);
    return mem;
#else
    return alloc->reallocFunc(alloc, p, oldSize, newSize);
#endif
#endif
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

void* M_ArrayInit(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, size_t elementSize, size_t minNeeded) {
    if (a) {
        MArrayHeader* p = M_ArrayHeader(a);
        p->size = 0;
        return M_ArrayGrow(MDEBUG_SOURCE_PASS alloc, a, p, elementSize, minNeeded);
    } else {
        u32 capacity = 8; // allocate for at least 8 elements
        if (capacity < minNeeded) {
            capacity = minNeeded;
        }
        size_t newSize =  elementSize * capacity + sizeof(MArrayHeader);
        void* mem = M_Malloc(MDEBUG_SOURCE_PASS alloc, newSize);
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

void* M_ArrayGrow(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, MArrayHeader* p, size_t elementSize, size_t minNeeded) {
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
    void* mem = M_Realloc(MDEBUG_SOURCE_PASS alloc, p, oldSize, newSize);
    if (mem == NULL)  {
        MLogf("M_ArrayGrow realloc failed for %p", a);
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

void M_ArrayFree(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, size_t itemSize) {
    MArrayHeader* p = M_ArrayHeader(a);
    M_Free(MDEBUG_SOURCE_PASS alloc, p, itemSize * p->capacity + sizeof(MArrayHeader));
}

/////////////////////////////////////////////////////////
// Mem buffer reading / writing

void M_MemResize(MMemIO* memIO, u32 newMinSize) {
    u32 newSize = newMinSize;
    if (memIO->capacity * 2 > newSize) {
        newSize = memIO->capacity * 2;
    }
    memIO->mem = MRealloc(memIO->allocator, memIO->mem, memIO->capacity, newSize);
    memIO->capacity = newSize;
}

void MMemInit(MMemIO *memIO, MAllocator* alloc, u8* mem, u32 size) {
    memIO->mem = mem;
    memIO->size = 0;
    memIO->capacity = size;
    memIO->allocator = alloc;
}

void MMemInitAlloc(MMemIO *memIO, MAllocator* alloc, u32 size) {
    u8* mem = (u8*) MMalloc(alloc, size);
    MMemInit(memIO, alloc, mem, size);
}

void MMemInitRead(MMemIO* memIO, u8 *mem, u32 size) {
    memIO->mem = mem;
    memIO->size = 0;
    memIO->capacity = size;
}

MINLINE void M_MemGrowBytes(MMemIO* memIO, u32 growByBytes) {
    size_t newSize = memIO->size + growByBytes;
    if (newSize > memIO->capacity) {
        M_MemResize(memIO, newSize);
    }
}

void MMemGrowBytes(MMemIO* memIO, u32 growByBytes) {
    M_MemGrowBytes(memIO, growByBytes);
}

u8* MMemAddBytes(MMemIO* memIO, u32 growBy) {
    M_MemGrowBytes(memIO, growBy);
    u8 *start = memIO->mem + memIO->size;
    memIO->size += growBy;
    return start;
}

u8* MMemAddBytesZero(MMemIO* memIO, u32 size) {
    M_MemGrowBytes(memIO, size);
    u8* start = memIO->mem + memIO->size;
    memset(start, 0, size);
    memIO->size += size;
    return start;
}

void MMemWriteI8(MMemIO* memIO, i8 val) {
    M_MemGrowBytes(memIO, 1);
    *(memIO->mem + memIO->size) = val;
    memIO->size += 1;
}

void MMemWriteU8(MMemIO* memIO, u8 val) {
    M_MemGrowBytes(memIO, 1);
    *(memIO->mem + memIO->size) = val;
    memIO->size += 1;
}

void MMemWriteU16BE(MMemIO* memIO, u16 val) {
    M_MemGrowBytes(memIO, 2);
    u8 a[] = {val >> 8, val & 0xff};
    memmove(memIO->mem + memIO->size, a, 2);
    memIO->size += 2;
}

void MMemWriteU16LE(MMemIO* memIO, u16 val) {
    M_MemGrowBytes(memIO, 2);
    u8 a[] = {val & 0xff, val >> 8};
    memmove(memIO->mem + memIO->size, a, 2);
    memIO->size += 2;
}

void MMemWriteI16BE(MMemIO* memIO, i16 val) {
    M_MemGrowBytes(memIO, 2);
    u8 a[] = {val >> 8, val & 0xff};
    memmove(memIO->mem + memIO->size, a, 2);
    memIO->size += 2;
}

void MMemWriteI16LE(MMemIO* memIO, i16 val) {
    M_MemGrowBytes(memIO, 2);
    u8 a[] = {val & 0xff, val >> 8};
    memmove(memIO->mem + memIO->size, a, 2);
    memIO->size += 2;
}

void MMemWriteU32BE(MMemIO* memIO, u32 val) {
    M_MemGrowBytes(memIO, 4);
    u8 a[] = {val >> 24, (val >> 16) & 0xff, (val >> 8) & 0xff, (val & 0xff)};
    memmove(memIO->mem + memIO->size, a, 4);
    memIO->size += 4;
}

void MMemWriteU32LE(MMemIO* memIO, u32 val) {
    M_MemGrowBytes(memIO, 4);
    u8 a[] = {(val & 0xff), (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24)};
    memmove(memIO->mem + memIO->size, a, 4);
    memIO->size += 4;
}

void MMemWriteI32BE(MMemIO* memIO, i32 val) {
    M_MemGrowBytes(memIO, 4);
    u8 a[] = {val >> 24, (val >> 16) & 0xff, (val >> 8) & 0xff, (val & 0xff)};
    memmove(memIO->mem + memIO->size, a, 4);
    memIO->size += 4;
}

void MMemWriteI32LE(MMemIO* memIO, i32 val) {
    M_MemGrowBytes(memIO, 4);
    u8 a[] = {(val & 0xff), (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24)};
    memmove(memIO->mem + memIO->size, a, 4);
    memIO->size += 4;
}

void MMemWriteU8CopyN(MMemIO* restrict memIO, u8* restrict src, u32 size) {
    if (size == 0) {
        return;
    }

    M_MemGrowBytes(memIO, size);
    memmove(memIO->mem + memIO->size, src, size);
    memIO->size += size;
}

void MMemWriteI8CopyN(MMemIO* restrict memIO, i8* restrict src, u32 size) {
    if (size == 0) {
        return;
    }

    M_MemGrowBytes(memIO, size);
    memmove(memIO->mem + memIO->size, src, size);
    memIO->size += size;
}

i32 MMemReadI8(MMemIO* restrict memIO, i8* restrict val) {
    if (memIO->size >= memIO->capacity) {
        return -1;
    }

    *(val) = *((i8*) (memIO->mem + memIO->size++));
    return 0;
}

i32 MMemReadU8(MMemIO* restrict memIO, u8* restrict val) {
    if (memIO->size >= memIO->capacity) {
        return -1;
    }

    *(val) = *(memIO->mem + memIO->size++);
    return 0;
}

i32 MMemReadI16(MMemIO* restrict memIO, i16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 2);
    memIO->size += 2;
    return 0;
}

i32 MMemReadI16BE(MMemIO* restrict memIO, i16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i16 v;
    memmove(&v, memIO->mem + memIO->size, 2);
    *(val) = MBIGENDIAN16(v);
#else
    memmove(val, memIO->mem + memIO->size, 2);
#endif
    memIO->size += 2;
    return 0;
}

i32 MMemReadI16LE(MMemIO* restrict memIO, i16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    i16 v;
    memmove(&v, memIO->mem + memIO->size, 2);
    *(val) = MLITTLEENDIAN16(v);
#else
    memmove(val, memIO->mem + memIO->size, 2);
#endif
    memIO->size += 2;
    return 0;
}

i32 MMemReadU16(MMemIO* restrict memIO, u16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 2);
    memIO->size += 2;
    return 0;
}

i32 MMemReadU16BE(MMemIO* restrict memIO, u16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    u16 v;
    memmove(&v, memIO->mem + memIO->size, 2);
    *(val) = MBIGENDIAN16(v);
#else
    memmove(val, memIO->mem + memIO->size, 2);
#endif
    memIO->size += 2;
    return 0;
}

i32 MMemReadU16LE(MMemIO* restrict memIO, u16* restrict val) {
    if (memIO->size + 2 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    u16 v;
    memmove(&v, memIO->mem + memIO->size, 2);
    *(val) = MLITTLEENDIAN16(v);
#else
    memmove(val, memIO->mem + memIO->size, 2);
#endif
    memIO->size += 2;
    return 0;
}

i32 MMemReadI32(MMemIO* restrict memIO, i32* restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 4);
    memIO->size += 4;
    return 0;
}

i32 MMemReadI32LE(MMemIO* restrict memIO, i32* restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->mem + memIO->size, 4);
    *(val) = MLITTLEENDIAN32(v);
#else
    memmove(val, memIO->mem + memIO->size, 4);
#endif
    memIO->size += 4;
    return 0;
}

i32 MMemReadI32BE(MMemIO* restrict memIO, i32* restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i32 v;
    memmove(&v, memIO->mem + memIO->size, 4);
    *(val) = MBIGENDIAN32(v);
#else
    memmove(val, memIO->mem + memIO->size, 4);
#endif
    memIO->size += 4;
    return 0;
}

i32 MMemReadU32(MMemIO* restrict memIO, u32* restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 4);
    memIO->size += 4;
    return 0;
}

i32 MMemReadU32LE(MMemIO* restrict memIO, u32 *restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->mem + memIO->size, 4);
    *(val) = MLITTLEENDIAN32(v);
#else
    memmove(val, memIO->mem + memIO->size, 4);
#endif
    memIO->size += 4;
    return 0;
}

i32 MMemReadU32BE(MMemIO* restrict memIO, u32* restrict val) {
    if (memIO->size + 4 > memIO->capacity) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i32 v;
    memmove(&v, memIO->mem + memIO->size, 4);
    *(val) = MBIGENDIAN32(v);
#else
    memmove(val, memIO->mem + memIO->size, 4);
#endif
    memIO->size += 4;
    return 0;
}

i32 MMemReadI64(MMemIO* memIO, i64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 8);
    memIO->size += 8;
    return 0;
}

i32 MMemReadI64BE(MMemIO* memIO, i64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

#ifdef MLITTLEENDIAN
    i64 v;
    memmove(&v, memIO->mem + memIO->size, 8);
    *(val) = MBIGENDIAN64(v);
#else
    memmove(val, memIO->mem + memIO->size, 8);
#endif
    memIO->size += 8;
    return 0;
}

i32 MMemReadI64LE(MMemIO* memIO, i64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    i32 v;
    memmove(&v, memIO->mem + memIO->size, 8);
    *(val) = MLITTLEENDIAN64(v);
#else
    memmove(val, memIO->mem + memIO->size, 8);
#endif
    memIO->size += 8;
    return 0;
}

i32 MMemReadU64(MMemIO* memIO, u64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

    memmove(val, memIO->mem + memIO->size, 8);
    memIO->size += 8;
    return 0;
}

i32 MMemReadU64BE(MMemIO* memIO, u64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    memmove(val, memIO->mem + memIO->size, 8);
#else
    u64 v;
    memmove(&v, memIO->mem + memIO->size, 8);
    *(val) = MBIGENDIAN64(v);
#endif
    memIO->size += 8;
    return 0;
}

i32 MMemReadU64LE(MMemIO* memIO, u64* val) {
    if (memIO->size + 8 > memIO->capacity) {
        return -1;
    }

#ifdef MBIGENDIAN
    i64 v;
    memmove(&v, memIO->mem + memIO->size, 8);
    *(val) = MLITTLEENDIAN64(v);
#else
    memmove(val, memIO->mem + memIO->size, 8);
#endif
    memIO->size += 8;
    return 0;
}

i32 MMemReadU8CopyN(MMemIO* memIO, u8* dest, u32 size) {
    if (memIO->size + size > memIO->capacity) {
        return -1;
    }
    memmove(dest, memIO->mem + memIO->size, size);
    memIO->size += size;
    return 0;
}

i32 MMemReadCharCopyN(MMemIO* memIO, char* dest, u32 size) {
    if (memIO->size + size > memIO->capacity) {
        return -1;
    }
    memmove(dest, memIO->mem + memIO->size, size);
    memIO->size += size;
    return 0;
}

i32 MMemReadCopy(MMemIO* reader, MMemIO* write, u32 size) {
    MMemGrowBytes(write, size);
    i32 r = MMemReadU8CopyN(reader, write->mem + write->size, size);
    if (r == 0) {
        write->size += size;
    }
    return r;
}

char* MMemReadStr(MMemIO* memIO) {
    u8 *oldPos = memIO->mem + memIO->size;
    while (*(memIO->mem + memIO->size)) { memIO->size++; }
    return (char*) oldPos;
}

/////////////////////////////////////////////////////////
// String parsing / conversion functions

i32 MParseI32(const char* start, const char* end, i32* out) {
    int val = 0;
    int begin = 1;
    int base = 10;

    for (const char* pos = start; pos < end; pos++) {
        char c = *pos;
        if (begin && MCharIsWhitespace(c)) {
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
        if (begin && MCharIsWhitespace(c)) {
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

i32 MParseBool(const char* start, const char* end, b32* out) {
    MStr str = {(char*)start, end - start};
    if (str.size == 5) {
        if (MStrCmp2("false", str) == 0) {
            *out = FALSE;
            return MParse_SUCCESS;
        }
        if (MStrCmp2("False", str) == 0) {
            *out = FALSE;
            return MParse_SUCCESS;
        }
        if (MStrCmp2("FALSE", str) == 0) {
            *out = FALSE;
            return MParse_SUCCESS;
        }
    } else if (str.size == 4) {
        if (MStrCmp2("true", str) == 0) {
            *out = TRUE;
            return MParse_SUCCESS;
        }
        if (MStrCmp2("True", str) == 0) {
            *out = TRUE;
            return MParse_SUCCESS;
        }
        if (MStrCmp2("TRUE", str) == 0) {
            *out = TRUE;
            return MParse_SUCCESS;
        }
    } else if (str.size == 1) {
        if (*str.str == '1') {
            *out = TRUE;
            return MParse_SUCCESS;
        }
        if (*str.str == '0') {
            *out = FALSE;
            return MParse_SUCCESS;
        }
    }
    return MParse_NOT_A_BOOL;
}

i32 MStrCmp(const char *str1, const char *str2) {
    while (*str1 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (*str1 > *str2) - (*str2 > *str1);
}

i32 MStrCmp2(const char *str1, MStr mstr2) {
    char c1 = *str1;
    if (mstr2.size == 0) {
        return c1 == 0 ? 0 : 1;
    }
    char *str2Ptr = mstr2.str;
    char *str2PtrEnd = mstr2.str + mstr2.size;

    while (TRUE) {
        char c2 = str2Ptr < str2PtrEnd ? *str2Ptr++ : 0;
        if (c1 != c2) {
            return c1 == 0 ? -1 : (c1 > c2 ? 1 : -1);
        }
        if (c1 == 0) {
            return 0;
        }
        c1 = *++str1;
    }
}

b32 MStrIsEmpty(MStr str) {
    if (str.size == 0 || str.str[0] == '\0') {
        return TRUE;
    }
    return FALSE;
}

i32 MStrCmp3(MStr str1, MStr str2) {
    if (MStrIsEmpty(str1) && MStrIsEmpty(str2)) {
        return 0;
    }

    char *str1Ptr = str1.str;
    char *str1PtrEnd = str1.str + str1.size;
    if (*(str1PtrEnd-1) == 0) {
        str1PtrEnd--;
    }

    char *str2Ptr = str2.str;
    char *str2PtrEnd = str2.str + str2.size;
    if (*(str2PtrEnd-1) == 0) {
        str2PtrEnd--;
    }

    while (str1Ptr < str1PtrEnd && str2Ptr < str2PtrEnd) {
        if (*str1Ptr != *str2Ptr) {
            return (*str1Ptr > *str2Ptr) ? 1 : -1;
        }
        str1Ptr++;
        str2Ptr++;
    }

    if (str1Ptr == str1PtrEnd && str2Ptr == str2PtrEnd) {
        return 0;
    }
    return (str1Ptr < str1PtrEnd) ? 1 : -1;
}

void MStrCopyN(char *dest, const char *src, size_t size) {
    if (!src) {
        *dest = 0;
        return;
    }

    size_t i;
    for (i = 0; i < size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = 0;
}

void MStrU32ToBinary(u32 val, int size, char* out) {
    out[size] = '\0';

    u32 z = (1 << (size - 1));
    for (int i = 0; i < size; i++, z >>= 1) {
        char c = ((val & z) == z) ? '1' : '0';
        out[i] = c;
    }
}

i32 MStrAppend(MMemIO* memIo, const char* str) {
    i32 len = MStrLen(str);

    i32 newSize = memIo->capacity + len + 1;
    if (newSize > memIo->capacity) {
        M_MemResize(memIo, newSize);
    }

    if (memIo->size) {
        memcpy(memIo->mem + memIo->size - 1, str, len + 1);
        memIo->size += len;
    } else {
        memcpy(memIo->mem, str, len + 1);
        memIo->size = len + 1;
    }

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

#if defined(M_STACKTRACE)

#if defined(M_LIBBACKTRACE)
#include <backtrace.h>
#include <stdio.h>

// One-time symbol backend init
static MOnce sLibBackTraceOnce = M_ONCE_INIT;
struct backtrace_state* sBacktraceState;

static void MLibBacktrace_ErrorCallback(void* data, const char* msg, int errnum) {
    (void)data;
    MLogf("LibBacktrace error: %s (%d)", msg, errnum);
}

MINTERNAL void MLibBacktraceInitFunc(void) {
    if (!sBacktraceState) {
        sBacktraceState = backtrace_create_state(NULL, 1, MLibBacktrace_ErrorCallback, NULL);
        if (sBacktraceState == NULL) {
            MLog("Failed to create backtrace state");
        }
    }
}

MINLINE void MLibBacktraceInit() {
    MExecuteOnce(&sLibBackTraceOnce, MLibBacktraceInitFunc);
}

static int MLogStacktrace_PrintBacktraceCallback(void* data, uintptr_t pc, const char* filename, int lineno,
    const char* function) {
    (void)data; // Avoid unused variable warning
    if (function) {
        MLogf("  %s() %s:%d", function, filename, lineno);
    } else if (filename != NULL && lineno != 0) {
        MLogf("  %?%?%?() %s:%d", filename, lineno);
    }
    return 0;
}

static void MLogStacktrace_ErrorCallback(void* data, const char* msg, int errnum) {
    (void)data;
    MLogf("LibBacktrace error: %s (%d)", msg, errnum);
}

void MLogStacktraceCurrent(i32 skipFrames) {
    MLibBacktraceInit();
    backtrace_full(sBacktraceState, skipFrames + 1, MLogStacktrace_PrintBacktraceCallback,
        MLogStacktrace_ErrorCallback, NULL);
}


void MLogStacktrace(MStacktrace* stacktrace) {
    MLibBacktraceInit();
    for (int i = 0; i < stacktrace->frames; i++) {
        void* address = stacktrace->addresses[i];
        backtrace_pcinfo(sBacktraceState, (uintptr_t)address, MLogStacktrace_PrintBacktraceCallback,
            MLogStacktrace_ErrorCallback, NULL);
    }
}

static int GetStacktraceHash_SimpleCallback(void* data, uintptr_t pc) {
    MStacktrace* stacktrace = (MStacktrace*)data;
    stacktrace->addresses[stacktrace->frames] = (void*)pc;
    stacktrace->hash = (stacktrace->hash * 31) + pc;
    stacktrace->frames += 1;
    return 0;
}

static void GetStacktraceHash_ErrorCallback(void* data, const char* msg, int errnum) {
    (void)data;
    MLogf("MGetStacktrace error: %s (%d)", msg, errnum);
}

b32 MGetStacktrace(MStacktrace* stacktrace, int skipFrames) {
    MLibBacktraceInit();
    stacktrace->hash = 0;
    stacktrace->frames = 0;
    backtrace_simple(sBacktraceState, skipFrames + 1, GetStacktraceHash_SimpleCallback,
        GetStacktraceHash_ErrorCallback, stacktrace);
    return TRUE;
}

#elif defined(WIN32)
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#ifdef M_THREADING
// Protect single threaded dbghelp functions
static MMutex gSymLock = M_MUTEX_INIT;
#endif

// One-time symbol backend init
static MOnce gSymOnce = M_ONCE_INIT;

MINTERNAL void MWin32SymInit(void) {
    HANDLE proc = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(proc, NULL, TRUE);
}

static void MWin32EnsureSymInit() {
    MExecuteOnce(&gSymOnce, MWin32SymInit);
}

void MLogStacktraceCurrent(int skipFrames) {
    CONTEXT context = {};
    RtlCaptureContext(&context);

    STACKFRAME64 stackFrame = {};
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rbp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;

    skipFrames += 1;

    MWin32EnsureSymInit();

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

#ifdef M_THREADING
    MMutexLock(&gSymLock);
#endif
    while (TRUE) {
        BOOL r = StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread,
                   &stackFrame, &context, NULL, SymFunctionTableAccess64,
                   SymGetModuleBase64, NULL);
        if (!r) {
#ifdef M_THREADING
            MMutexUnlock(&gSymLock);
#endif
            break;
        }

        if (skipFrames) {
            skipFrames--;
            continue;
        }

        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 address = stackFrame.AddrPC.Offset;
        DWORD64 displacement = 0;
        char* funcName = NULL;
        char* fileName = NULL;
        u32 lineNo = 0;

        if (SymFromAddr(process, address, &displacement, symbol)) {
            funcName = symbol->Name;
        }

        IMAGEHLP_LINE64 lineInfo = {};
        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo)) {
            fileName = lineInfo.FileName;
            lineNo = lineInfo.LineNumber;
        }

#ifdef M_THREADING
        MMutexUnlock(&gSymLock);
#endif

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

#ifdef M_THREADING
        MMutexLock(&gSymLock);
#endif
    }
}

void MLogStacktrace(MStacktrace* stacktrace) {
    MWin32EnsureSymInit();

    HANDLE process = GetCurrentProcess();

    for (int i = 0; i < stacktrace->frames; ++i) {
        void* pc = stacktrace->addresses[i];

#ifdef M_THREADING
        MMutexLock(&gSymLock);
#endif
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO) symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 address = (DWORD64)pc;
        DWORD64 displacement = 0;
        char* funcName = NULL;
        char* fileName = NULL;
        u32 lineNo = 0;

        if (SymFromAddr(process, address, &displacement, symbol)) {
            funcName = symbol->Name;
        }

        IMAGEHLP_LINE64 lineInfo = {};
        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo)) {
            fileName = lineInfo.FileName;
            lineNo = lineInfo.LineNumber;
        }

#ifdef M_THREADING
        MMutexUnlock(&gSymLock);
#endif

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
}

b32 MGetStacktrace(MStacktrace* stacktrace, int skipFrames) {
    skipFrames += 1;
    u64 hash = 0;
    ULONG framesToCapture = MStaticArraySize(stacktrace->addresses);
    if (framesToCapture + skipFrames >= 63) {
        framesToCapture = 63 - skipFrames;
    }
    USHORT frames = CaptureStackBackTrace(skipFrames, framesToCapture,
        stacktrace->addresses, NULL);
    for (USHORT i = 0; i < frames; i++) {
        hash = (hash * 31) + (u64) stacktrace->addresses[i];
    }

    stacktrace->frames = frames;
    stacktrace->hash = (u32)(hash & 0xFFFFFFFF);

    return TRUE;
}

#elif defined(__GLIBC__) || defined(__APPLE__)
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

b32 MGetStacktrace(MStacktrace* stacktrace, int skipFrames) {
    skipFrames+=1;

    void* array[64];
    int frames = backtrace(array, MStaticArraySize(array));

    u64 hash = 0;
    stacktrace->frames = 0;

    for (size_t i = skipFrames; i < frames && stacktrace->frames < MStaticArraySize(stacktrace->addresses); i++) {
        stacktrace->addresses[stacktrace->frames] = array[i];
        hash = (hash * 31) + (u64)array[i];
        stacktrace->frames++;
    }

    stacktrace->hash = (u32)(hash & 0xFFFFFFFF);
    return TRUE;
}

void MLogStacktrace(MStacktrace* stacktrace) {
    char** strings = backtrace_symbols(stacktrace->addresses, stacktrace->frames);
    if (strings) {
        for (int i = 0; i < stacktrace->frames; i++) {
            MLogf("  %s", strings[i]);
        }
        free(strings);
    }
}

void MLogStacktraceCurrent(int skipFrames) {
    skipFrames += 1;
    void* array[64];
    int frames = backtrace(array, MStaticArraySize(array));

    char** strings;
    strings = backtrace_symbols(array, frames);
    for (size_t i = skipFrames; i < frames; i++) {
        MLogf("  %s", strings[i]);
    }

    free(strings);
}
#endif

#endif
