#ifndef PROFILER_H
#define PROFILER_H

typedef struct prof_ctx prof_ctx;
struct prof_ctx
{
	Arena *arena;
	String8 formula;
	Instruction inst;
};

typedef u8 (*profilerFuncPtr)(prof_ctx *ctx); 

void run_8086_profiler(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u8 StartFlag);
void print_inst_profile_with_explain(Arena *arena, s32 fd, u8 clocks, u64 total_clocks, String8 formula);
void print_inst_profile(Arena *arena, s32 fd, u8 clocks, u64 total_clocks);

#endif