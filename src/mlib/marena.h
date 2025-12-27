#pragma once

#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Arena bump allocator with optional chaining
// Call MArenaReset() to free() all the allocations made in one go.
// To checkpoint and free allocations made after the checkpoint, call MArenaCheckpoint() and MArenaResetToCheckpoint().

// Block of memory for putting arena allocations
// The memory the block references is directly after the block structure
typedef struct MArenaBlock {
    size_t size;
    size_t usedBytes;
    struct MArenaBlock *prev; // Prev allocated block
    struct MArenaBlock *next; // Next allocated block
} MArenaBlock;

typedef struct MArenaBlockCheckpoint {
    u8 *pos;
#ifdef M_MEM_DEBUG
    // Debug info for checkpoints
    // Since we do allocation tracking
    MMemAllocInfo* allocSlots;
    u32* freeSlots;
#ifdef M_STACKTRACE
    MStacktrace* stacktraces;
#endif
#endif
} MArenaBlockCheckpoint;

// Arena when memory can be allocated from
// Memory can only be free()ed all in one go
// Two modes:
// 1. Bump allocate from a fixed address range
// 2. Dynamically add memory blocks as more memory is requested
typedef struct MArena {
    MAllocator alloc;
    u8* mem;
    u8* end;
    size_t size;
    u32 alignBytes; // alignment in bytes - must be a non-zero power of two

    // Dynamic chain block allocation
    // Note that the underlying baseAlloc is expected to return an int multiple of given alignBytes
    MArenaBlock* curBlock;
    MAllocator* baseAlloc;
} MArena;

typedef struct MArenaStats {
    u32 numBlocks;
    size_t usedBytes;
    size_t capacity;
} MArenaStats;

/**
 * Initialize to use pre-allocated memory of a given size.
 * When memory runs out, any additional MMalloc will return NULL.
 * @param mem Pointer to memory to use
 * @param size Size of memory to use
 * @param alignBytes alignment of pointers returned by MMalloc/MRealloc
 */
void MArenaInitFixed(MArena* arena, u8* mem, size_t size, size_t alignBytes);

/**
 * Create a growable arena
 * Memory is allocated in blocks, new blocks grow 2x from the previous block
 * @param allocator Allocator to allocate blocks.  This allocator should return pointers that are an integer multiple
 * alignBytes.
 * @param blockSize Initial block size in bytes.
 * @param alignBytes Alignment of pointers returned by MMalloc/MRealloc.
 */
b32 MArenaInitGrowable(MArena* arena, MAllocator* allocator, size_t blockSize, size_t alignBytes);

/**
 * Free the allocator (use only when allocated with MArenaInitGrowable()
 * Frees the underlying blocks
 */
void MArenaFreeGrowable(MArena* arena);

/**
 * Reset allocation tracking for arena.  Call this when you want to discard all allocations made to the arena in one go.
 * Does not free underlying blocks
 */
void MArenaReset(MArena* arena);

/**
 * Create a checkpoint of current arena allocation state.
 * The returned MArenaBlockCheckpoint can be passed to MArenaResetToCheckpoint() to free all allocations after the
 * checkpoint in one go.
 */
MArenaBlockCheckpoint MArenaCheckpoint(MArena *arena);

/**
 * Reset arena allocation state back to a previously created checkpoint
 * Blocks remain allocated
 * @param arena Arena to reset
 * @param checkpoint Previously created checkpoint to reset to
 */
void MArenaResetToCheckpoint(MArena* arena, MArenaBlockCheckpoint checkpoint);

/**
 * Get stats for arena
 */
MArenaStats MArenaGetStats(MArena* arena);

/**
 * Print some debug info about the arena
 */
void MArenaDumpInfo(MArena* arena);

// MAllocator callbacks - don't need to call directly
void* MArenaMalloc(void* arena, size_t size);
void* MArenaRealloc(void* arena, void* ptr, size_t oldSize, size_t newSize);
void MArenaFree(void* arena, void* ptr, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif
