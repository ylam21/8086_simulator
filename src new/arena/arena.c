#include "arena.h"

Arena *arena_create(u64 size)
{
    Arena *arena;
    arena = malloc(sizeof(Arena));
    if (arena == NULL)
    {
        perror("malloc");
        return NULL;
    }
    arena->buffer = malloc(size);
    if (arena->buffer == NULL)
    {
        perror("malloc");
        free(arena);
        return NULL;
    }
    arena->cap = size;
    arena->pos = 0;
    return arena;
}

static u64 align_forward(u64 ptr, u64 align)
{
    return (ptr + align - 1) & ~(align - 1);
}

void  *arena_push(Arena *arena, u64 size)
{
    u64 current_ptr = (u64)arena->buffer + arena->pos;
    u64 offset = align_forward(current_ptr, DEFAULT_ALIGNMENT);
    offset -= (u64)arena->buffer;

    if (offset + size <= arena->cap)
    {
        void *ptr = (u8*)arena->buffer + offset;
        arena->pos  = offset + size;
        memset(ptr, 0, size);
        return ptr;
    }

    return NULL;
}

void *arena_push_packed(Arena *arena, u64 size)
{
    if (arena->pos + size <= arena->cap)
    {
        void *ptr = (u8*)arena->buffer + arena->pos;
        arena->pos += size; 
        memset(ptr, 0, size);
        return ptr;
    }
    return NULL;
}

void arena_reset(Arena *arena)
{
    if (arena)
    {
        arena->pos = 0;
    }
}

void arena_destroy(Arena *arena)
{
    if (arena)
    {
        free(arena->buffer);
        free(arena);
    }
}