#ifndef PROFILER_H
#define PROFILER_H

typedef u8 (*profilerFuncPtr)(Instruction inst); 

void run_8086_profiler(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u32 StartFlags);

#endif