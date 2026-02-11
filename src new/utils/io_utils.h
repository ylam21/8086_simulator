#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "../common.h"
#include "../decoder/opcodes.h"

void write_fmt_line(t_ctx *ctx, u8 *mnemonic, u8 *operands);
void write_fmt_line_no_operands(t_ctx *ctx, u8 *mnemonic);

#endif