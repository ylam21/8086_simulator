#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "../common.h"
#include "unistd.h"

s16 string8_decimal_to_s16(String8 s);
s32 string8_hex_to_s16(String8 s);
u8 is_whitespace(u8 c);
u8 is_space(u8 c);
u8 is_alpha(u8 c);
u8 is_num(u8 c);
u8 is_alnum(u8 c);

void write_final_regs(Arena *arena, s32 fd, u16 regs[8]);
void write_line(Arena *arena, s32 fd, String8 line, String8 dest, u16 reg_start, u16 regs_end);

#endif