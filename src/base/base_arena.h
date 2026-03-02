#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#define DEFAULT_ALIGNMENT (u64)sizeof(void *)

typedef struct Arena Arena;
struct Arena
{
    void *buffer;
    u64 cap;
    u64 pos;
};

Arena *arena_create(u64 size);
void *arena_push(Arena *arena, u64 size);
void *arena_push_packed(Arena *arena, u64 size);
void arena_reset(Arena *arena);
void arena_destroy(Arena *arena);

#endif
