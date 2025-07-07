#ifndef MLIB_H
#define MLIB_H

//
// Public domain c library for the following:
//
// - heap debugging
// - c arrays
// - memory/file reading / writing framework
// - ini reading
//
// You might want check out STB libs and GCC heap debug options before using this, this is just my personal version that
// integrates custom heap debugging/tracking with a custom stb style array.
//
#include <stdio.h>
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

// Mark function as internal (used to distinguish between variables that are static)
#define MINTERNAL static

// Mark function as internal and hint to compiler to make it inline (typically gcc will already once marked
// internal, if appropriate, without having to add 'inline')
#define MINLINE static inline

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

//////////////////////////////////////////////////////////
// Memory allocation
// As an alternative GCC has good compile time options for similar checks

typedef void* (*M_malloc_t)(void* alloc, size_t size);
typedef void* (*M_realloc_t)(void* alloc, void* mem, size_t oldSize, size_t newSize);
typedef void (*M_free_t)(void* alloc, void* mem, size_t size);

typedef struct sMAllocator {
    M_malloc_t mallocFunc;
    M_realloc_t reallocFunc;
    M_free_t freeFunc;
} MAllocator;

// Set the default allocator, if not set stdlib will be used
void MMemAllocSet(MAllocator* allocator);

#ifndef M_CLIB_DISABLE
void MMemUseClibAllocator();
#endif

//////////////////////////////////////////////////////////
// Memory allocation tracking & debugging
// GCC has good compile time options for similar checks, if you want to stick to using GCC & its clib(s)

#ifdef M_MEM_DEBUG
// Memory debug mode is on
#define MDEBUG_SOURCE_DEFINE const char* file, int line,
#define MDEBUG_SOURCE_PASS file, line,
#define MDEBUG_SOURCE_MACRO __FILE__, __LINE__,

void MMemDebugInit();
void MMemDebugDeinit();

// Check if ptr is a valid memory allocation, returns false if there's a problem
b32 MMemDebugCheck(void* p);

// Check if all memory allocations are okay - checks for write overruns / underruns using sentinel values
// Prints any errors.  Returns false if there's a problem, true if okay.
b32 MMemDebugCheckAll();

// Print all current memory allocations
b32 MMemDebugListAll();
#else
// Memory debug mode is off
#define MDEBUG_SOURCE_DEFINE
#define MDEBUG_SOURCE_PASS
#define MDEBUG_SOURCE_MACRO
#endif

void* _MMalloc(MDEBUG_SOURCE_DEFINE size_t size);
void* _MMallocZ(MDEBUG_SOURCE_DEFINE size_t size);
void* _MRealloc(MDEBUG_SOURCE_DEFINE void* p, size_t oldSize, size_t newSize);
void* _MReallocZ(MDEBUG_SOURCE_DEFINE void* p, size_t oldSize, size_t newSize);
void _MFree(MDEBUG_SOURCE_DEFINE void* p, size_t size);

#define MMalloc(size) _MMalloc(MDEBUG_SOURCE_MACRO size)
#define MMallocZ(size) _MMallocZ(MDEBUG_SOURCE_MACRO size)
#define MRealloc(p, oldSize, newSize) (_MRealloc(MDEBUG_SOURCE_MACRO (p), oldSize, newSize))
#define MFree(p, size) (_MFree(MDEBUG_SOURCE_MACRO (p), size), (p) = NULL)

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

// Log / trigger debugger
#ifdef M_ASSERT
#ifdef __GNUC__
#if defined(__i386__) || defined(__x86_64)
#define MBreakpoint(str) { MLog(str); asm("int $3"); }
#define MBreakpointf(str, ...) { MLogf(str, __VA_ARGS__); asm("int $3"); }
#define MAssert(cond, str) { if (!(cond)) { MLog(str); asm("int $3"); } }
#define MAssertf(cond, str, ...)  { if (!(cond)) { MLogf(str, __VA_ARGS__); asm("int $3"); } }
#else
#define MBreakpoint(str) MLog(str)
#define MBreakpointf(str, ...) MLogf(str, __VA_ARGS__)
#define MAssert(cond, str) { if (!(cond)) { MLog(str); } }
#define MAssertf(cond, str, ...)  { if (!(cond)) { MLogf(str, __VA_ARGS__); } }
#endif
#endif
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
//
typedef struct {
    u8* mem;  // pointer to start of memory buffer
    u8* pos;  // current pos in memory buffer
    u32 size; // current size read or written (redundant since: size == pos - mem)
    u32 capacity; // size of memory buffer allocated in bytes
} MMemIO;

// Initialise MMemIO to write to existing memory
// size is set to zero and pos set to 'mem'
void MMemInit(MMemIO* memIO, u8* mem, u32 capacity);

// Initialise MMemIO to read from existing memory
// capacity and size are initialised to the same value
// and pos is the start of memory
void MMemReadInit(MMemIO* memIO, u8* mem, u32 size);

// Allocate the given amount of memory and
void MMemInitAlloc(MMemIO* memIO, u32 size);

MINTERNAL void MMemReset(MMemIO* memIO) {
    memIO->size = 0;
    memIO->pos = memIO->mem;
}

MINTERNAL void MMemFree(MMemIO* memIO) {
    MFree(memIO->mem, memIO->capacity);
}

// Add space for bytes, but don't advance used space
void MMemGrowBytes(MMemIO* memIO, u32 capacity);

// Add bytes, allocating new memory if necessary
// Returns point to start of newly added space for given number of bytes
u8* MMemAddBytes(MMemIO* memIO, u32 size);

// Add bytes, allocating new memory if necessary, zero out bytes added
// Returns point to start of newly added space for given number of bytes
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

MINTERNAL b32 MMemReadDone(MMemIO* reader) {
    u32 read = reader->pos - reader->mem;
    if (read >= reader->size) {
        return 1;
    } else {
        return 0;
    }
}

MINLINE i32 MMemReadSkipBytes(MMemIO* reader, u32 skipBytes) {
    reader->pos += skipBytes;
    return MMemReadDone(reader);
}

MINLINE void* MPtrAlign(void* ptr, size_t alignBytes) {
    return (void*)((uintptr_t)((u8*)ptr + alignBytes - 1) & ~(alignBytes - 1));
}

MINLINE size_t MSizeAlign(size_t size, size_t alignBytes) {
    return (size + alignBytes - 1) & ~(alignBytes - 1);
}

#define MStaticArraySize(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct sMArena {
    void* mem;
    void* pos;
    size_t capacity;
} MArena;

/////////////////////////////////////////////////////////
// Dynamic Array
// Pretty much the stb lib array type, but integrates with heap debug functionality above.

typedef struct {
    size_t size; // number of elements currently used
    size_t capacity; // total capacity in array elements
} MArrayHeader;

// Init array of given size
#define MArrayInit(a, s) ((a) = M_ArrayInit(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), s))

// Use this to free the memory allocated
#define MArrayFree(a) ((a) ? M_ArrayFree(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a)), (a) = NULL : 0)

// Add value to end of array
#define MArrayAdd(a, v) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), 1), (a)[M_ArrayHeader(a)->size++] = (v))

// Add space for item to end of array and return ptr to it
#define MArrayAddPtr(a) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), 1), ((a) + M_ArrayHeader(a)->size++))

// Grow the array, new elements are uninitialized, old elements remain set as before (i.e. they are memmove()d if the
// backing memory needs to relocated to fit the new elements, and left alone if not).
#define MArrayGrow(a, s) ((a) = M_ArrayMaybeGrow(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), s))

// Insert a value at given index to an array (may involve moving memory and/or reallocation)
#define MArrayInsert(a, i, v) ((a) = M_ArrayInsertSpace(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), (i)), ((a)[(i)] = (v)), 0)

// Set value in array, resize if necessary, gaps are initialized to zero.
#define MArraySet(a, i, v) ((a) = M_ArrayGrowAndClearIfNeeded(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), (i) + 1)), (((a)[(i)] = (v)), 0)

#define MArraySize(a) ((a) ? M_ArrayHeader(a)->size : 0)
#define MArrayLen(a) (MArraySize(a))

#define MArrayCapacity(a) ((a) ? M_ArrayHeader(a)->capacity : 0)

// Pop array item from end of array, and return a copy of it
#define MArrayPop(a) ((a)[--(M_ArrayHeader(a)->size)])

#define MArrayTop(a) ((a)[(M_ArrayHeader(a)->size) - 1])

#define MArrayTopPtr(a) ((a) + (M_ArrayHeader(a)->size) - 1)

#define MArrayClear(a) ((a) ? M_ArrayHeader(a)->size = 0 : 0)

#define MArrayCopy(a, b) ((b) = M_ArrayCopy(MDEBUG_SOURCE_MACRO M_ArrayUnpack(a), M_ArrayUnpack(b)))

#define MArrayRemoveIndex(a, b) (memcpy((a)+(b), (a)+(b)+1, (MArraySize(a)-(b)-1)*(sizeof*(a))))

#define MArrayEach(a, i) for (size_t i = 0; (i) < MArraySize(a); ++(i))

#define MArrayEachPtr(a, it) for (struct {typeof (a) p; size_t i;} (it) = {(a), 0}; ((it).i) < MArraySize(a); ++((it).i), (it).p = (a) + (it.i))

#define M_ArrayHeader(a) ((MArrayHeader*)(a) - 1)

// Helper macro to convert array ptr to (T* arr, size_t elementSize)
#define M_ArrayUnpack(a) (a), sizeof *(a)

// Grow array to have enough space for at least minNeeded elements
// If it fails (OOM), the array will be deleted, a.p will be NULL and the functions returns 0;
// else (on success) it returns 1
void* M_ArrayInit(MDEBUG_SOURCE_DEFINE void* a, size_t elementSize, size_t minNeeded);
void* M_ArrayGrow(MDEBUG_SOURCE_DEFINE void* a, MArrayHeader* p, size_t elementSize, size_t minNeeded);
void M_ArrayFree(MDEBUG_SOURCE_DEFINE void* a, size_t elementSize);

MINLINE void* M_ArrayMaybeGrow(MDEBUG_SOURCE_DEFINE void* a, size_t elementSize, size_t numAdd) {
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
    return M_ArrayGrow(MDEBUG_SOURCE_PASS a, p, elementSize, needed);
}

MINLINE void* M_ArrayCopy(MDEBUG_SOURCE_DEFINE void* src, size_t srcElementSize, void* dest, size_t destElementSize) {
    if (src) {
        MArrayHeader* srcHeader = M_ArrayHeader(src);
        dest = M_ArrayMaybeGrow(MDEBUG_SOURCE_PASS dest, destElementSize, srcHeader->size);
        MArrayHeader* destHeader = M_ArrayHeader(dest);
        destHeader->size = srcHeader->size;
        memcpy(dest, src, srcElementSize * srcHeader->size);
    } else {
        dest = NULL;
    }
    return dest;
}

MINLINE void* M_ArrayGrowAndClearIfNeeded(MDEBUG_SOURCE_DEFINE void* arr, size_t elementSize, size_t newSize) {
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
        void* b = M_ArrayGrow(MDEBUG_SOURCE_PASS arr, p, elementSize, newSize);
        memset((u8*)b + (oldSize * elementSize), 0, (newSize - oldSize) * elementSize);
        p = M_ArrayHeader(arr);
        p->size = newSize;
        return b;
    }
}

MINLINE void* M_ArrayInsertSpace(MDEBUG_SOURCE_DEFINE void* arr, size_t elementSize, size_t i) {
    void* r = M_ArrayMaybeGrow(MDEBUG_SOURCE_PASS arr, elementSize, 1);
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

#define MFileReadFully(filePath) (MFileReadWithOffset(filePath, 0, 0))

#define MFileReadWithSize(filePath, readSize) (MFileReadWithOffset(filePath, 0, readSize))

MReadFileRet MFileReadWithOffset(const char* filePath, u32 offset, u32 readSize);

i32 MFileWriteDataFully(const char* filePath, u8* data, u32 size);

typedef struct {
    void* handle;
    u32 open;
} MFile;

MFile MFileWriteOpen(const char* filePath);
i32 MFileWriteData(MFile* file, u8* data, u32 size);
MINLINE i32 MFileWriteMem(MFile* file, MMemIO* mem) {
    return MFileWriteData(file,  mem->mem, mem->pos - mem->mem);
}
void MFileClose(MFile* file);

/////////////////////////////////////////////////////////
// String parsing / conversion functions

typedef struct {
    char* str;
    u32 size;
} MStr;

#define MStrInit(s, len) ((s).str = (char*)_MMalloc(MDEBUG_SOURCE_MACRO (len)), ((s).str) ? (s).size = (len) : 0)
#define MStrFree(s) ((s).str ? (_MFree(MDEBUG_SOURCE_MACRO (s).str, (s).size), (s).str = 0, (s).size = 0) : 0)

enum MParse {
    MParse_SUCCESS = 0,
    MParse_NOT_A_NUMBER = -1,
};

i32 MParseI32(const char* start, const char* end, i32* out);
i32 MParseI32Hex(const char* start, const char* end, i32* out);
i32 MStrCmp(const char* str1, const char* str2);
i32 MStrCmp3(const char* str1, const char* str2Start, const char* str2End);
void MStrU32ToBinary(u32 val, i32 size, char* out);

// Get pointer to end of str (scan for zero).  This is useful for functions that take a function end.
const char* MStrEnd(const char* str);

MINTERNAL u32 MStrLen(const char* str) {
    if (str == NULL) {
        return 0;
    }
    u32 n = 0;
    while (*str++) n++;
    return n;
}

MINLINE MStr MStrMake(const char* c) {
    MStr r = {};
    u32 len = MStrLen(c) + 1;
    MStrInit(r, len);
    memcpy(r.str, c, len);
    return r;
}

i32 MStringAppend(MMemIO* memIo, const char* str);
i32 MStringAppendf(MMemIO* memIo, const char* format, ...);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
