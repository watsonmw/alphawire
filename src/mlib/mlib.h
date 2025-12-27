#pragma once

//
// Public domain c library for the following:
//
// - heap debugging
// - arrays
// - memory block reader / writers
// - strings
// - logging
// - threading
// - stacktraces
//
// You might want to check out STB libs and GCC heap debug options before using this.
//
// Feature defines:
//
//  * M_ASSERT           * Asserts & breakpoints
//  * M_LOG_ALLOCATIONS  * Log allocations
//  * M_MEM_DEBUG        * Heap checking
//  * M_STACKTRACE       * Enable Stacktraces
//   - M_LIBBACKTRACE     - Use libbacktrace
//  * M_THREADING        * Enable threading / thread local / mutexes
//   - M_PTHREADS         - Use pthreads
//
#include <string.h>

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef int b32;
typedef unsigned u32;
typedef long long i64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

#define I8_MIN 0x80
#define I8_MAX 0x7f
#define U8_MAX 0xff
#define I16_MIN 0x8000
#define I16_MAX 0x7fff
#define U16_MAX 0xffff
#define I32_MIN 0x80000000
#define I32_MAX 0x7fffffff
#define U32_MAX 0xffffffff
#define I64_MIN 0x8000000000000000
#define I64_MAX 0x7fffffffffffffff
#define U64_MAX 0xffffffffffffffff

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Mark a function as internal (used to distinguish between variables that are static)
#define MINTERNAL static

// Mark a function as internal and hint to compiler to make it inline (typically gcc will already once marked
// internal, if appropriate, without having to add 'inline')
#define MINLINE static inline

#ifdef __cplusplus
extern "C" {
#endif

// Turn off some warnings that will get triggered if -Wall is on
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-function"
    #define M_DISABLE_ANON_STRUCT_WARN_B
    #define M_DISABLE_ANON_STRUCT_WARN_E
#elif defined(_MSC_VER)
    // Macros to disable C4116 "unnamed type definition in parentheses" when using MArrayEachPtr
    #define M_DISABLE_ANON_STRUCT_WARN_B __pragma(warning(push)) __pragma(warning(disable: 4116))
    #define M_DISABLE_ANON_STRUCT_WARN_E __pragma(warning(pop))
#else
    #define M_DISABLE_ANON_STRUCT_WARN_B
    #define M_DISABLE_ANON_STRUCT_WARN_E
#endif

// Get type of variable. Useful in macros to get the type of a parameter
#ifdef _MSC_VER
    #define M_TYPEOF(a) __typeof__(a)
#else
    #define M_TYPEOF(a) typeof(a)
#endif

//////////////////////////////////////////////////////////
// Memory allocation

typedef void* (*M_malloc_t)(void* alloc, size_t size);
typedef void* (*M_realloc_t)(void* alloc, void* mem, size_t oldSize, size_t newSize);
typedef void (*M_free_t)(void* alloc, void* mem, size_t size);

#ifdef M_STACKTRACE
typedef struct {
    void* addresses[64];
    u32 frames;
    u32 hash;
} MStacktrace;
#endif

#ifdef M_MEM_DEBUG
typedef struct {
    size_t size;
    u8* start;
    u8* mem;
    const char* file;
    int line;
#ifdef M_STACKTRACE
    u32 stacktraceHash;
#endif
} MMemAllocInfo;

typedef struct {
    MMemAllocInfo* allocSlots;
    u32* freeSlots; // Indices of available allocSlots
#ifdef M_STACKTRACE
    MStacktrace* stacktraces;
#endif
    u32 initialized : 1;
    u32 sentinelCheck : 1;
    u32 leakTracking : 1;
    u32 totalAllocations;
    u32 totalAllocatedBytes;
    u32 curAllocatedBytes;
    u32 maxAllocatedBytes;
} MMemDebugContext;
#endif

typedef struct {
    M_malloc_t mallocFunc;
    M_realloc_t reallocFunc;
    M_free_t freeFunc;
    char* name;
#ifdef M_MEM_DEBUG
    MMemDebugContext debug;
#endif
} MAllocator;

// Set the default allocator, if not set stdlib will be used
void MMemAllocSet(MAllocator* alloc);

#ifndef M_CLIB_DISABLE
void MAllocatorMakeClibHeap(MAllocator* alloc);
#endif

//////////////////////////////////////////////////////////
// Memory allocation tracking & debugging
// GCC has good compile time options for similar checks, if you want to stick to using GCC & its clib(s)

#ifdef M_MEM_DEBUG
// Memory debug mode is on
#define MDEBUG_SOURCE_DEFINE const char* file, int line,
#define MDEBUG_SOURCE_PASS file, line,
#define MDEBUG_SOURCE_MACRO __FILE__, __LINE__,

void MMemDebugInit(MAllocator* alloc);
void MMemDebugDeinit(MAllocator* alloc);
void MMemDebugDeinit2(MAllocator* alloc, b32 logSummary);

// Check if ptr is a valid memory allocation, returns false if there's a problem
b32 MMemDebugCheck(MAllocator* alloc, void* p);

// Check if all memory allocations are okay - checks for write overruns / underruns using sentinel values
// Prints any errors.  Returns false if there's a problem, true if okay.
b32 MMemDebugCheckAll(MAllocator* alloc);

// Print all current memory allocations
b32 MMemDebugListAll(MAllocator* alloc);

// Free all allocated pointers in the given address range
// Compacts the allocation slots and frees the slot list
// Used when tracking a bump allocator
void MMemDebugFreePtrsInRange(MAllocator* alloc, const u8* startAddress, const u8* endAddress);

#else
// Memory debug mode is off
#define MDEBUG_SOURCE_DEFINE
#define MDEBUG_SOURCE_PASS
#define MDEBUG_SOURCE_MACRO
#endif

void* M_Malloc(MDEBUG_SOURCE_DEFINE MAllocator* alloc, size_t size);
void* M_Realloc(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* p, size_t oldSize, size_t newSize);
void M_Free(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* p, size_t size);
MINLINE void* M_MallocZ(MDEBUG_SOURCE_DEFINE MAllocator* alloc, size_t size) {
    void* r = M_Malloc(MDEBUG_SOURCE_MACRO (alloc), size);
    if (r) {
        memset(r, 0, size);
    }
    return r;
}
MINLINE void* M_ReallocZ(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* p, size_t oldSize, size_t newSize) {
    void* r = M_Realloc(MDEBUG_SOURCE_MACRO (alloc), p, oldSize, newSize);
    if (r && newSize > oldSize) {
        memset(((u8*)r)+oldSize, 0, newSize - oldSize);
    }
    return r;
}

#define MMalloc(alloc, size) M_Malloc(MDEBUG_SOURCE_MACRO (alloc), size)
#define MMallocZ(alloc, size) M_MallocZ(MDEBUG_SOURCE_MACRO (alloc), size)
#define MRealloc(alloc, p, oldSize, newSize) (M_Realloc(MDEBUG_SOURCE_MACRO (alloc), (p), oldSize, newSize))
#define MReallocZ(alloc, p, oldSize, newSize) (M_ReallocZ(MDEBUG_SOURCE_MACRO (alloc), (p), oldSize, newSize))
#define MFree(alloc, p, size) (M_Free(MDEBUG_SOURCE_MACRO (alloc), (p), size), (p) = NULL)

/////////////////////////////////////////////////////////
// Threading & locking
#ifdef M_THREADING

// Define variable as thread local.
#if defined(__GNUC__)
    #define MTHREAD_LOCAL __thread
#elif defined(_MSC_VER)
    #define MTHREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define thread_local _Thread_local
#else
    #define MTHREAD_LOCAL
#endif

#if defined(M_PTHREADS)
    #define M_MUTEX_INIT {0}
    typedef struct { int m; } MMutex;

    #define M_ONCE_INIT 0
    typedef int MOnce;
#elif defined(_WIN32)
    #define M_MUTEX_INIT {0}
    typedef struct {void* m;} MMutex;

    #define M_ONCE_INIT {0}
    typedef struct {void* o;} MOnce;
#endif

void MMutexLock(MMutex* m);
void MMutexUnlock(MMutex* m);
#else

#define M_ONCE_INIT 0
typedef int MOnce;
void MExecuteOnce(MOnce* once, void (*fn)(void));
#endif

/////////////////////////////////////////////////////////
// Math
#define MSWAP(x, y, T) { T SWAP = x; x = y; y = SWAP; }

/////////////////////////////////////////////////////////
// Endian conversions
// 16 flip: ((t & 0xff) << 8) + ((t >> 8) & 0xff)
// 32 flip: ((t & 0xff) << 24) + ((t >> 24) & 0xff) + ((t >> 16 & 0xff) << 8) + (((t >> 8) & 0xff) << 16);
#ifdef __GNUC__
    #define MSTRUCTPACKED(...) __VA_ARGS__ __attribute__((packed))
    #ifdef AMIGA
        #define MBIGENDIAN 1
        #define MBIGENDIAN16(x) x
        #define MBIGENDIAN32(x) x
        #define MBIGENDIAN64(x) x
        #define MLITTLEENDIAN16(x) __builtin_bswap16(x)
        #define MLITTLEENDIAN32(x) __builtin_bswap32(x)
        #define MLITTLEENDIAN64(x) __builtin_bswap64(x)
        #define MENDIANSWAP16(x) __builtin_bswap16(x)
        #define MENDIANSWAP32(x) __builtin_bswap32(x)
        #define MENDIANSWAP64(x) __builtin_bswap64(x)
    #else
        #define MLITTLEENDIAN 1
        #define MBIGENDIAN16(x) __builtin_bswap16(x)
        #define MBIGENDIAN32(x) __builtin_bswap32(x)
        #define MBIGENDIAN64(x) __builtin_bswap64(x)
        #define MLITTLEENDIAN16(x) x
        #define MLITTLEENDIAN32(x) x
        #define MLITTLEENDIAN64(x) x
        #define MENDIANSWAP16(x) __builtin_bswap16(x)
        #define MENDIANSWAP32(x) __builtin_bswap32(x)
        #define MENDIANSWAP64(x) __builtin_bswap64(x)
    #endif
#elif defined(_MSC_VER)
    #define MLITTLEENDIAN 1
    #define MSTRUCTPACKED(...) \
        __pragma(pack(push, 1)) \
        __VA_ARGS__ \
        __pragma(pack(pop))
    #define MBIGENDIAN16(x) _byteswap_ushort(x)
    #define MBIGENDIAN32(x) _byteswap_ulong(x)
    #define MBIGENDIAN64(x) _byteswap_uint64(x)
    #define MLITTLEENDIAN16(x) x
    #define MLITTLEENDIAN32(x) x
    #define MLITTLEENDIAN64(x) x
    #define MENDIANSWAP16(x) _byteswap_ushort(x)
    #define MENDIANSWAP32(x) _byteswap_ulong(x)
    #define MENDIANSWAP64(x) _byteswap_uint64(x)
#else
    #define MBIGENDIAN 1
    #define MSTRUCTPACKED(...) __VA_ARGS__
    #define MBIGENDIAN16(x) x
    #define MBIGENDIAN32(x) x
    #define MBIGENDIAN64(x) x
    #define MENDIANSWAP16(t) (((t & 0xff) << 8) + ((t >> 8) & 0xff))
    #define MENDIANSWAP32(t) (((t & 0xff) << 24) + ((t >> 24) & 0xff) + ((t >> 16 & 0xff) << 8) + (((t >> 8) & 0xff) << 16))
    #define MENDIANSWAP64(t) (((t & 0xff) << 56) + ((t >> 8 & 0xff) << 48) + \
        (((t >> 16) & 0xff) << 40) + (((t >> 24) & 0xff) << 32) + \
        (((t >> 32) & 0xff) << 24) + (((t >> 40) & 0xff) << 16) + \
        ((t >> 56) & 0xff))
    #define MLITTLEENDIAN16(t) MENDIANSWAP16(t)
    #define MLITTLEENDIAN32(t) MENDIANSWAP32(t)
    #define MLITTLEENDIAN64(t) MENDIANSWAP64(t)
#endif

/////////////////////////////////////////////////////////
// Logging

void MLogf(const char *format, ...);
void MLogfNoNewLine(const char *format, ...);
void MLog(const char *str);
void MLogBytes(const u8* mem, u32 len);

// Log & trigger debugger
#ifdef M_ASSERT
    #if defined(__GNUC__)
        #if defined(__clang__)
            #define M_DEBUGGER_TRAP() __builtin_debugtrap()
        #else
            #if defined(__i386__) || defined(__x86_64)
                #define M_DEBUGGER_TRAP() __asm__ volatile("int $3")
            #elif defined(__arm__) || defined(__aarch64__)
                #define M_DEBUGGER_TRAP() __asm__ volatile("bkpt #0")
            #endif
        #endif
    #elif defined(_MSC_VER)
        #define M_DEBUGGER_TRAP() __debugbreak()
    #endif
    #ifndef M_DEBUGGER_TRAP
        #define M_DEBUGGER_TRAP() ((void)0)
    #endif
    #define MBreakpoint(str) { MLog(str); M_DEBUGGER_TRAP(); }
    #define MBreakpointf(str, ...) { MLogf(str, __VA_ARGS__); M_DEBUGGER_TRAP(); }
    #define MAssert(cond, str) { if (!(cond)) { MLog(str); M_DEBUGGER_TRAP(); } }
    #define MAssertf(cond, str, ...) { if (!(cond)) { MLogf(str, __VA_ARGS__); M_DEBUGGER_TRAP(); } }
#else
    #define MBreakpoint(str)
    #define MBreakpointf(str, ...)
    #define MAssert(cond, str)
    #define MAssertf(cond, str, ...)
#endif

// Quote the given #define contents, this is useful to print out the contents of a macro / #define.
// E.g. if BUILD_INFO is passed in to compiler from command line with -DBUILD_INFO=<build info>, you
// can add the following to the C file to allow the version to be printed out / pulled from static
// data in final exe:
//   const char* BUILD_STRING = MMACRO_QUOTE(BUILD_INFO);
#define __MMACRO_QUOTE(name) #name
#define MMACRO_QUOTE(name) __MMACRO_QUOTE(name)

/////////////////////////////////////////////////////////
// Memory reading / writing
typedef struct {
    u8* mem;  // pointer to start of memory buffer
    u32 size; // current size in bytes written/read
    u32 capacity; // size of memory buffer allocated in bytes
    MAllocator* allocator;
} MMemIO;

// Initialise MMemIO to write to existing memory
void MMemInit(MMemIO* memIO, MAllocator* alloc, u8* mem, u32 capacity);

// Initialise MMemIO for write - no initial allocation
MINLINE void MMemInitEmpty(MMemIO* memIO, MAllocator* alloc) {
    memIO->size = 0;
    memIO->mem = NULL;
    memIO->capacity = 0;
    memIO->allocator = alloc;
}

// Initialise MMemIO for write - allocate 'size' bytes for writing
void MMemInitAlloc(MMemIO* memIO, MAllocator* alloc, u32 size);

// Initialise MMemIO to read from existing memory
void MMemInitRead(MMemIO* memIO, u8* mem, u32 size);

MINLINE void MMemReset(MMemIO* memIO) {
    memIO->size = 0;
}

MINLINE void MMemFree(MMemIO* memIO) {
    MFree(memIO->allocator, memIO->mem, memIO->capacity);
    memIO->size = 0;
    memIO->capacity = 0;
}

// Add capacity to write growByBytes after the current position (grow memory if the additional bytes don't fit in the
// remaining capacity)
void MMemGrowBytes(MMemIO* memIO, u32 growByBytes);

// Add bytes, allocating new memory if necessary
// Returns pointer to start of newly added space for the given number of bytes
u8* MMemAddBytes(MMemIO* memIO, u32 size);

// Add bytes, allocating new memory if necessary, zero out bytes added
// Returns point to start of newly added space for the given number of bytes
u8* MMemAddBytesZero(MMemIO* memIO, u32 size);

// --- Writing ---
// Write data at current pos and advance.
// These are often slower than writing directly as they do endian byte swaps and allow writing at byte offsets.
// For speed call MMemAddBytes() and write data directly to *pos, making sure the offsets are aligned.
void MMemWriteI8(MMemIO* memIO, i8 val);
void MMemWriteU8(MMemIO* memIO, u8 val);

void MMemWriteI16LE(MMemIO* memIO, i16 val);
void MMemWriteI16BE(MMemIO* memIO, i16 val);
void MMemWriteU16LE(MMemIO* memIO, u16 val);
void MMemWriteU16BE(MMemIO* memIO, u16 val);

void MMemWriteI32LE(MMemIO* memIO, i32 val);
void MMemWriteI32BE(MMemIO* memIO, i32 val);
void MMemWriteU32LE(MMemIO* memIO, u32 val);
void MMemWriteU32BE(MMemIO* memIO, u32 val);

void MMemWriteU8CopyN(MMemIO* writer, u8* src, u32 size);
void MMemWriteI8CopyN(MMemIO* writer, i8* src, u32 size);

// --- Reading ---
// Read data at current pos and advance.
i32 MMemReadI8(MMemIO* reader, i8* val);
i32 MMemReadU8(MMemIO* reader, u8* val);

i32 MMemReadI16(MMemIO* reader, i16* val);
i32 MMemReadI16BE(MMemIO* reader, i16* val);
i32 MMemReadI16LE(MMemIO* reader, i16* val);

i32 MMemReadU16(MMemIO* reader, u16* val);
i32 MMemReadU16BE(MMemIO* reader, u16* val);
i32 MMemReadU16LE(MMemIO* reader, u16* val);

i32 MMemReadI32(MMemIO* reader, i32* val);
i32 MMemReadI32BE(MMemIO* reader, i32* val);
i32 MMemReadI32LE(MMemIO* reader, i32* val);

i32 MMemReadU32(MMemIO* reader, u32* val);
i32 MMemReadU32BE(MMemIO* reader, u32* val);
i32 MMemReadU32LE(MMemIO* reader, u32* val);

i32 MMemReadI64(MMemIO* reader, i64* val);
i32 MMemReadI64BE(MMemIO* reader, i64* val);
i32 MMemReadI64LE(MMemIO* reader, i64* val);

i32 MMemReadU64(MMemIO* reader, u64* val);
i32 MMemReadU64BE(MMemIO* reader, u64* val);
i32 MMemReadU64LE(MMemIO* reader, u64* val);

i32 MMemReadU8CopyN(MMemIO* reader, u8* dst, u32 size);
i32 MMemReadCharCopyN(MMemIO* reader, char* dst, u32 size);

i32 MMemReadCopy(MMemIO* reader, MMemIO* out, u32 size);

// Read a null terminated string
char* MMemReadStr(MMemIO* reader);

MINLINE b32 MMemReadDone(MMemIO* reader) {
    if (reader->size >= reader->capacity) {
        return TRUE;
    } else {
        return FALSE;
    }
}

MINLINE i32 MMemReadSkipBytes(MMemIO* reader, u32 skipBytes) {
    reader->size += skipBytes;
    return MMemReadDone(reader);
}

MINLINE u8* MMemReadAdvance(MMemIO* reader, u32 skipBytes) {
    u8* pos = reader->mem + reader->size;
    reader->size += skipBytes;
    return pos;
}

// Make pointer align to given given alignment (move pointer forward to make it align)
MINLINE void* MPtrAlign(void* ptr, size_t alignBytes) {
    return (void*)((uintptr_t)((u8*)ptr + alignBytes - 1) & ~(alignBytes - 1));
}

// Return byte size nearest multiple of alignBytes that size will fit in
MINLINE size_t MSizeAlign(size_t size, size_t alignBytes) {
    return (size + alignBytes - 1) & ~(alignBytes - 1);
}

// Get number of items in a static array
#define MStaticArraySize(arr) (sizeof(arr) / sizeof(arr[0]))

/////////////////////////////////////////////////////////
// Dynamic Array
// Pretty much the stb lib array type, but integrates with heap debug functionality above.

typedef struct {
    size_t size; // number of elements currently used
    size_t capacity; // total capacity in array elements
} MArrayHeader;

// Init array of given size
#define MArrayInit(m, a, s) ((a) = M_ArrayInit(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), s))

// Use this to free the memory allocated
#define MArrayFree(m, a) ((a) ? M_ArrayFree(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a)), (a) = NULL : 0)

// Add value to the end of the array
#define MArrayAdd(m, a, v) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), 1), (a)[M_ArrayHeader(a)->size++] = (v))

// Add space for item to the end of the array and return ptr to it
#define MArrayAddPtr(m, a) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), 1), ((a) + M_ArrayHeader(a)->size++))

// Add space for item to the end of the array, zero its memory and return ptr to it
#define MArrayAddPtrZ(m, a) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), 1), memset((a) + M_ArrayHeader(a)->size, 0, sizeof(*a)), ((a) + M_ArrayHeader(a)->size++))

#define MArrayClear(a) ((a) ? M_ArrayHeader(a)->size = 0 : 0)

// Resize array to given size. New elements are uninitialized.
#define MArrayResize(m, a, s) ((a) ? ((M_ArrayHeader(a)->size < (s)) ? ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), s), 0) : ((M_ArrayHeader(a)->size) = (s))) : 0)

// Grow the array, adding new elements. New elements are uninitialized, old elements are unchanged but may be moved (they are
// memmove()d if the backing memory needs to be relocated to fit the new elements, and left alone if not).
#define MArrayGrow(m, a, s) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), s))

// Insert a value at the given index to an array (may involve moving memory and/or reallocation)
#define MArrayInsert(m, a, i, v) ((a) = M_ArrayInsertSpace(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), (i)), ((a)[(i)] = (v)), 0)

// Set value in the array, resize if necessary, gaps are initialized to zero.
#define MArraySet(m, a, i, v) ((a) = M_ArrayGrowAndClearIfNeeded(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), (i) + 1)), (((a)[(i)] = (v)), 0)

#define MArraySize(a) ((a) ? M_ArrayHeader(a)->size : 0)
#define MArrayLen(a) (MArraySize(a))

#define MArrayCapacity(a) ((a) ? M_ArrayHeader(a)->capacity : 0)

// Pop an item from the end of the array and return a copy of it
#define MArrayPop(a) ((a)[--(M_ArrayHeader(a)->size)])

#define MArrayLast(a) ((a)[(M_ArrayHeader(a)->size) - 1])

#define MArrayLastPtr(a) (M_ArrayHeader(a)->size ? (a) + (M_ArrayHeader(a)->size) - 1 : NULL)

#define MArrayCopy(m, a, b) ((b) = M_ArrayCopy(MDEBUG_SOURCE_MACRO (m), M_ArrayUnpack(a), M_ArrayUnpack(b)))

#define MArrayRemoveIndex(a, b) (memcpy((a)+(b), (a)+(b)+1, (MArraySize(a)-(b)-1)*(sizeof*(a))))

#define MArrayEach(a, i) for (size_t i = 0; (i) < MArraySize(a); ++(i))

#define MArrayEachPtr(a, it) for ( \
    M_DISABLE_ANON_STRUCT_WARN_B struct {M_TYPEOF(a) p; size_t i;} M_DISABLE_ANON_STRUCT_WARN_E \
    (it) = {(a), 0}; ((it).i) < MArraySize(a); ++((it).i), ++((it).p))

#define M_ArrayHeader(a) ((MArrayHeader*)(a) - 1)

// Helper macro to convert array ptr to (T* arr, size_t elementSize)
#define M_ArrayUnpack(a) (a), sizeof *(a)

// Grow array to have enough space for at least minNeeded elements
// If it fails (OOM), the array will be deleted, a.p will be NULL and the function returns 0
// else (on success) it returns 1
void* M_ArrayInit(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, size_t elementSize, size_t minNeeded);
void* M_ArrayGrow(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, MArrayHeader* p, size_t elementSize, size_t minNeeded);
void M_ArrayFree(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, size_t elementSize);

MINLINE void* M_ArrayMaybeGrow(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* a, size_t elementSize, size_t numAdd) {
    size_t needed;
    MArrayHeader* p;
    if (a) {
        p = M_ArrayHeader(a);
        needed = p->size + numAdd;
        if (p->capacity >= needed) {
            return a;
        }
    } else {
        p = NULL;
        needed = numAdd;
    }
    return M_ArrayGrow(MDEBUG_SOURCE_PASS alloc, a, p, elementSize, needed);
}

MINLINE void* M_ArrayCopy(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* src, size_t srcElementSize, void* dest, size_t destElementSize) {
    if (src) {
        MArrayHeader* srcHeader = M_ArrayHeader(src);
        dest = M_ArrayMaybeGrow(MDEBUG_SOURCE_PASS alloc, dest, destElementSize, srcHeader->size);
        MArrayHeader* destHeader = M_ArrayHeader(dest);
        destHeader->size = srcHeader->size;
        memcpy(dest, src, srcElementSize * srcHeader->size);
    } else {
        dest = NULL;
    }
    return dest;
}

MINLINE void* M_ArrayGrowAndClearIfNeeded(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* arr, size_t elementSize, size_t newSize) {
    MArrayHeader* p = M_ArrayHeader(arr);
    if (p->size >= newSize) {
        return arr;
    }
    if (p->capacity >= newSize) {
        size_t oldSize = p->size;
        memset((u8*)arr + (oldSize * elementSize), 0, (newSize - oldSize) * elementSize);
        p->size = newSize;
        return arr;
    } else {
        u32 oldSize = p->size;
        void* b = M_ArrayGrow(MDEBUG_SOURCE_PASS alloc, arr, p, elementSize, newSize);
        memset((u8*)b + (oldSize * elementSize), 0, (newSize - oldSize) * elementSize);
        p = M_ArrayHeader(arr);
        p->size = newSize;
        return b;
    }
}

MINLINE void* M_ArrayInsertSpace(MDEBUG_SOURCE_DEFINE MAllocator* alloc, void* arr, size_t elementSize, size_t i) {
    void* r = M_ArrayMaybeGrow(MDEBUG_SOURCE_PASS alloc, arr, elementSize, 1);
    if (!r) {
        return r;
    }
    MArrayHeader* p = M_ArrayHeader(r);
    if (p->size != i) {
        u8* ptr = (u8*)r;
        u8* src = ptr + elementSize * i;
        u8* dest = src + elementSize;
        u32 size = (p->size - i) * elementSize;
        memmove(dest, src, size);
    }
    p->size++;
    return r;
}


/////////////////////////////////////////////////////////
// File Reading / Writing

typedef struct {
    u8* data;
    u32 size;
} MReadFileRet;

#define MFileReadFully(alloc, filePath) (MFileReadWithOffset(alloc, filePath, 0, 0))
#define MFileReadWithSize(alloc, filePath, readSize) (MFileReadWithOffset(alloc, filePath, 0, readSize))

MReadFileRet MFileReadWithOffset(MAllocator* alloc, const char* filePath, u32 offset, u32 readSize);
i32 MFileWriteDataFully(const char* filePath, u8* data, u32 size);

typedef struct {
    void* handle;
    u32 open;
} MFile;

MFile MFileWriteOpen(const char* filePath);
i32 MFileWriteData(MFile* file, u8* data, u32 size);
MINLINE i32 MFileWriteMem(MFile* file, MMemIO* mem) {
    return MFileWriteData(file, mem->mem, mem->size);
}
void MFileClose(MFile* file);

/////////////////////////////////////////////////////////
// Strings

// View onto a string - you never have to free these
// Rarely 0 terminated and size doesn't include 0 if it is
typedef struct MStrView {
    char* str;
    u32 size;
} MStrView;

// String storage - these need to be MFree()'d if capacity is non-zero
// Rarely 0 terminated and size doesn't include 0 if it is
typedef struct MStr {
    char* str;
    u32 size;
    u32 capacity; // size of memory buffer allocated in bytes, when 0 don't MFree()
} MStr;

enum MParse {
    MParse_SUCCESS = 0,
    MParse_NOT_A_NUMBER = -1,
    MParse_NOT_A_BOOL = -2,
    MParse_FLOAT_TWO_DOTS = -3,
    MParse_EMPTY = -4
};

typedef struct MParseResult {
    i32 result;
    const char* end;
} MParseResult;

MParseResult MParseI32(const char* start, const char* end, i32* out);
MParseResult MParseI32Hex(const char* start, const char* end, i32* out);
MParseResult MParseF32(const char* start, const char* end, f32* out);
MParseResult MParseBool(const char* pos, const char* end, b32* out);

MINLINE int MCharIsDigit(char c) { return (c >= '0' && c <= '9'); }
MINLINE int MCharIsWhitespace(char c) { return (c == '\t' || c == ' '); }
MINLINE int MCharIsSpaceTab(char c) { return (c == '\t' || c == ' '); }
MINLINE int MCharIsNewLine(char c) { return (c == '\n' || c == '\r'); }

// Get pointer to end of str (scan for null terminator)
MINLINE const char* MCStrEnd(const char* str) {
    if (!str) {return NULL;}
    while (*str) {str++;}
    return str;
}

MINLINE char* MStrEnd(MStr str) {
    return str.str + str.size;
}

MINLINE u32 MCStrLen(const char* str) {
    if (str == NULL) {return 0;}
    u32 n = 0;
    while (*str++) {n++;}
    return n;
}

MINLINE MStrView MStrViewWrap(const char* str) {
    return (MStrView){(char*)str, MCStrLen(str)};
}

MINLINE MStrView MStrViewFromStr(MStr str) {
    return (MStrView){str.str, str.size};
}

MINLINE b32 MStrIsEmpty(MStr str) {
    return str.str == 0 || str.size == 0;
}

MINLINE b32 MStrViewIsEmpty(MStrView str) {
    return str.str == 0 || str.size == 0;
}

MINLINE b32 MStrViewEq(MStrView* a, MStrView* b) {
    return (a->size == b->size && memcmp(a->str, b->str, a->size) == 0);
}

MINLINE u32 MStrViewLen(MStrView str) {
    if (str.str == NULL) {return 0;}
    return str.size;
}

MINLINE const char* MStrViewEnd(MStrView str) {
    return str.str + str.size;
}

i32 MCStrCmp(const char* str1, const char* str2);
i32 MStrViewCmpC(MStrView str1, const char* str2);
i32 MStrViewCmp(MStrView str1, MStrView str2);
MINLINE i32 MStrCmp(MStr str1, MStr str2) {
    return MStrViewCmp((MStrView){str1.str, str1.size}, (MStrView){str2.str, str2.size});
}
void MCStrCopyN(char* dest, const char* src, size_t size);
void MCStrU32ToBinary(u32 val, i32 outSize, char* outStr);

// Alloc space for string of given size
#define MStrInit(a, s, len) ((s).str = (char*)M_Malloc(MDEBUG_SOURCE_MACRO (a), (len)), \
    (s).size = (s).capacity = ((s).str) ? (len) : 0)

// Free allocated string
#define MStrFree(a, s) ((s).capacity ? (M_Free(MDEBUG_SOURCE_MACRO (a), (void*)(s).str, (s).capacity), \
    (s).str = 0, (s).size = 0, (s).capacity = 0) : 0)

// Make a MStr copy from a C string - includes null terminator
MINLINE MStr MStrMake(MAllocator* alloc, const char* c) {
    MStr r = {};
    u32 len = MCStrLen(c);
    MStrInit(alloc, r, len + 1);
    if (!r.str) {return r;}
    memcpy((void*)r.str, c, len + 1);
    r.size = len;
    r.capacity = len + 1;
    return r;
}

MINLINE MStr MStrMakeLen(MAllocator* alloc, const char* c, u32 len) {
    MStr r = {};
    MStrInit(alloc, r, len + 1);
    if (!r.str) {return r;}
    memcpy((void*)r.str, c, len + 1);
    r.size = len;
    r.capacity = len + 1;
    return r;
}

// Make a MStr from a C string
MINLINE MStr MStrWrap(const char* c) {
    const MStr r = {(char*)c, MCStrLen(c), 0};
    return r;
}

MINLINE void MStrZero(MStr* str) {
    str->str = NULL; str->size = str->capacity = 0;
}

MINLINE i32 MStrViewCopyN(MStrView* strView, char* bufOut, size_t bufOutSize) {
    size_t copySize = strView->size < bufOutSize - 1 ? strView->size : bufOutSize - 1;
    memcpy(bufOut, strView->str, copySize);
    bufOut[copySize] = '\0';
    return (i32)copySize;
}

u32 MStrAppend(MMemIO* memIo, const char* str);
u32 MStrAppendf(MMemIO* memIo, const char* format, ...);


/////////////////////////////////////////////////////////
// Stacktraces
#ifdef M_STACKTRACE
b32 MGetStacktrace(MStacktrace* stacktrace, int skipFrames);
void MLogStacktrace(MStacktrace* stacktrace);
void MLogStacktraceCurrent(int skipFrames);
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
} // extern "C"
#endif
