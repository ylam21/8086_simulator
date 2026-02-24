#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "../common.h"
#include "unistd.h"

void write_fmt_line(Arena *arena, s32 fd, String8 mnemonic, String8 operands);
void write_fmt_line_no_operands(Arena *arena, s32 fd, String8 mnemonic);
void write_fmt_sim_line(Arena *arena, s32 fd, String8 dest, u16 reg_old, u16 reg_new);
void write_final_regs(Arena *arena, s32 fd, u16 regs[14]);
u32 str8ncmp(String8 s1, String8 s2, u32 n);


#endif