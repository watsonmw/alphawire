#include "marena.h"

static size_t GetBlockHeaderSize(MArena* arena) {
    size_t blockHeaderSize = MSizeAlign(sizeof(MArenaBlock), arena->alignBytes);
    return blockHeaderSize;
}

// Find the next free block to use or create one if there is no block
static void MArenaPushNextBlock(MArena* arena, size_t minSize) {
    size_t blockHeaderSize = GetBlockHeaderSize(arena);
    size_t size = minSize + blockHeaderSize;

    size_t newAllocSize = arena->size * 2;
    if (size > newAllocSize) {
        newAllocSize = size;
    }

    MArenaBlock* curBlock = arena->curBlock;
    if (curBlock && curBlock->next) {
        if (newAllocSize >= curBlock->next->size) {
            // Use next block if it is big enough
            curBlock = curBlock->next;
        } else {
            // Otherwise realloc the block to the bigger size
            curBlock->next->prev = curBlock;
            curBlock = MRealloc(arena->baseAlloc, curBlock, curBlock->size, newAllocSize);
            if (curBlock == NULL) {
                MLogf("MArenaMalloc(%d) failed: out of memory", newAllocSize);
                return;
            }
            curBlock->size = newAllocSize;
            curBlock->prev = arena->curBlock;
        }
        arena->size = curBlock->size;
        arena->mem = (u8*)curBlock;
        arena->end = arena->mem + blockHeaderSize;
        arena->curBlock = curBlock;
        return;
    }

    MArenaBlock* newBlock = MMalloc(arena->baseAlloc, newAllocSize);
    memset(newBlock, 0, sizeof(*newBlock));
    if (newBlock == NULL) {
        MLogf("MArenaMalloc(%d) failed: out of memory", newAllocSize);
        return;
    }

    newBlock->size = newAllocSize;
    newBlock->prev = curBlock;
    if (curBlock) {
        curBlock->next = newBlock;
    }
    arena->size = newAllocSize;
    arena->mem = (u8*)newBlock;
    arena->end = arena->mem + blockHeaderSize;
    arena->curBlock = newBlock;
}

void* MArenaMalloc(void* _arena, size_t size) {
    MArena* arena = (MArena*)_arena;

    size = MSizeAlign(size, arena->alignBytes);
    u8* pos = arena->end;
    u8* newEnd = pos + size;
    size_t memUsed = newEnd - arena->mem;
    if (arena->size < memUsed) {
        if (arena->baseAlloc) {
            arena->curBlock->usedBytes = (arena->end - arena->mem) - GetBlockHeaderSize(arena);
            MArenaPushNextBlock(arena, size);
            return MArenaMalloc(arena, size);
        } else {
            MLogf("MArenaMalloc(%d) failed: out of memory - using %zu of %zu", size, memUsed, arena->size);
        }
        return NULL;
    }

    arena->end = newEnd;
    return MPtrAlign(pos, arena->alignBytes);
}

void* MArenaRealloc(void* _arena, void* mem, size_t oldSize, size_t newSize) {
    if (oldSize >= newSize) {
        return mem;
    }

    u8* newMem = MArenaMalloc(_arena, newSize);
    if (mem != NULL &&  oldSize > 0) {
        memcpy(newMem, mem, oldSize);
    }
    return newMem;
}

void MArenaFree(void* _ba, void* mem, size_t size) {
    // nop
}

#ifdef M_MEM_DEBUG
void InitMemDebug(MArena *arena) {
    MMemDebugInit(&arena->alloc);
    // Don't report leaks - we still track memory allocations in this mode - that's just for debugging which allocations
    // are made where.
    arena->alloc.debug.leakTracking = FALSE;
    // arena->alloc.debug.sentinelCheck = FALSE;
}
#endif

void MArenaInitFixed(MArena* arena, u8* mem, size_t size, size_t alignBytes) {
    memset(arena, 0, sizeof(MArena));
    arena->mem = mem;
    arena->end = MPtrAlign(mem, alignBytes);
    arena->size = size;
    arena->alignBytes = alignBytes;
    arena->alloc.mallocFunc = MArenaMalloc;
    arena->alloc.reallocFunc = MArenaRealloc;
    arena->alloc.freeFunc = MArenaFree;
    arena->alloc.name = "arena";
#ifdef M_MEM_DEBUG
    InitMemDebug(arena);
#endif
}

b32 MArenaInitGrowable(MArena* arena, MAllocator* allocator, size_t blockSize, size_t alignBytes) {
    memset(arena, 0, sizeof(MArena));
    arena->baseAlloc = allocator;
    arena->alignBytes = alignBytes;
    arena->alloc.mallocFunc = MArenaMalloc;
    arena->alloc.reallocFunc = MArenaRealloc;
    arena->alloc.freeFunc = MArenaFree;
    arena->alloc.name = "arena";

    size_t blockHeaderSize = MSizeAlign(sizeof(MArenaBlock), arena->alignBytes);
    if (blockSize < blockHeaderSize) {
        MLogf("MArenaInitGrowable failed size too small %zu", blockSize);
        return FALSE;
    }

    MArenaBlock* block = MMalloc(arena->baseAlloc, blockSize);
    memset(block, 0, sizeof(*block));
    if (block == NULL) {
        MLogf("MArenaInitGrowable(%zu) failed: unable to alloc memory from base allocator", blockSize);
        return FALSE;
    }

    block->size = blockSize;
    arena->mem = (u8*)block;
    arena->end = arena->mem + blockHeaderSize;
    arena->size = blockSize;
    arena->curBlock = block;

#ifdef M_MEM_DEBUG
    InitMemDebug(arena);
#endif

    return TRUE;
}

void MArenaFreeGrowable(MArena* arena) {
#ifdef M_MEM_DEBUG
    MMemDebugDeinit2(&arena->alloc, FALSE);
#endif
    MArenaBlock* block = arena->curBlock;
    if (block) {
        block = arena->curBlock->next;
        while (block) {
            MArenaBlock* nextBlock = block->next;
            MFree(arena->baseAlloc, block, block->size);
            block = nextBlock;
        }
        block = arena->curBlock;
        while (block) {
            MArenaBlock* prevBlock = block->prev;
            MFree(arena->baseAlloc, block, block->size);
            block = prevBlock;
        }
    }
    arena->curBlock = NULL;
    arena->mem = NULL;
    arena->end = NULL;
    arena->size = 0;
}

MArenaBlockCheckpoint MArenaCheckpoint(MArena* arena) {
    MArenaBlockCheckpoint checkpoint = {
        arena->end,
#ifdef M_MEM_DEBUG
        arena->alloc.debug.allocSlots,
        arena->alloc.debug.freeSlots,
        arena->alloc.debug.stacktraces
#endif
    };
    return checkpoint;
}

void MArenaResetToCheckpoint(MArena* arena, MArenaBlockCheckpoint checkpoint) {
    MArenaBlock *block = arena->curBlock;
    arena->alloc.debug.allocSlots = checkpoint.allocSlots;
    arena->alloc.debug.freeSlots = checkpoint.freeSlots;
    arena->alloc.debug.stacktraces = checkpoint.stacktraces;
    while (block) {
        u8* blockStart = (u8*)block;
        u8* blockEnd = (u8*)block + block->size;
        if (blockStart <= checkpoint.pos && checkpoint.pos <= blockEnd) {
#ifdef M_MEM_DEBUG
            // Free memory slots after the checkpoint
            MMemDebugFreePtrsInRange(&arena->alloc, checkpoint.pos, blockEnd);
#endif
            break;
        }
        arena->curBlock = block->prev;
#ifdef M_MEM_DEBUG
        // Free all memory slots in this block (it's entirely post checkpoint)
        MMemDebugFreePtrsInRange(&arena->alloc, blockStart, blockEnd);
#endif
        block = arena->curBlock;
    }
    if (block) {
        arena->mem = (u8 *) block;
        arena->size = block->size;
        arena->end = checkpoint.pos;
    }
}

void MArenaReset(MArena* arena) {
    MArenaBlock* block = arena->curBlock;
    while (block) {
        arena->curBlock = block;
        block = block->prev;
    }
    size_t blockHeaderSize = MSizeAlign(sizeof(MArenaBlock), arena->alignBytes);
    arena->mem = (u8*)arena->curBlock;
    arena->size = arena->curBlock->size;
    arena->end = ((u8*)arena->curBlock) + blockHeaderSize;
#ifdef M_MEM_DEBUG
    // Memory is going to be reused, make sure debug structures are also free()ed from the memory
    MMemDebugDeinit2(&arena->alloc, FALSE);
    // Initialize again because we called MMemDebugDeinit()
    InitMemDebug(arena);
#endif
}

MArenaStats MArenaGetStats(MArena* arena) {
    MArenaStats stats = {};
    MArenaBlock* block = arena->curBlock;

    stats.usedBytes += (arena->end - arena->mem);
    if (block) {
        size_t blockHeaderSize = GetBlockHeaderSize(arena);
        stats.usedBytes -= blockHeaderSize;
        stats.numBlocks = 1;
    }
    stats.capacity += arena->size;

    while (block) {
        MArenaBlock* prevBlock = block->prev;
        if (block != arena->curBlock) {
            stats.usedBytes += block->usedBytes;
            stats.capacity += block->size;
            stats.numBlocks += 1;
        }
        block = prevBlock;
    }

    return stats;
}

void MArenaDumpInfo(MArena* arena) {
    MArenaStats stats = MArenaGetStats(arena);

    MLogf("Arena Info:");
    if (stats.capacity) {
        MLogf("  Size: %zu out of %zu bytes (%d%%)", stats.usedBytes, stats.capacity,
            stats.usedBytes * 100 / stats.capacity);
    } else {
        MLogf("  Size: %zu out of %zu bytes", stats.usedBytes, stats.capacity);
    }

    MArenaBlock* block = arena->curBlock;
    if (block) {
        int i = 0;
        // Used or partially used blocks
        while (block) {
            MLogf("  + Block %d: size: %zu", i, block->size);
            block = block->prev;
            i++;
        }
        // Free blocks not currently used
        block = arena->curBlock->next;
        while (block) {
            MLogf("  - Block %d: size: %zu", i, block->size);
            block = block->next;
            i++;
        }
    }
}
