#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "../common.h"
#include "../decoder/opcodes.h"
#include "../decoder/decoder.h"
#include "unistd.h"

void write_fmt_line(t_ctx *ctx, String8 mnemonic, String8 operands);
void write_fmt_line_no_operands(t_ctx *ctx, String8 mnemonic);
void write_fmt_sim_line(t_ctx *ctx, String8 dest, u16 reg_old, u16 reg_new);
void write_final_regs(Arena *arena, s32 fd, u16 regs[14]);


#endif