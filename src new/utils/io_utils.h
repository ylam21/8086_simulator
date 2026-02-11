#ifndef IO_UTILS_H
#define IO_UTILS_H

#include "../common.h"
#include "../decoder/opcodes.h"
#include "unistd.h"

void write_fmt_line(t_ctx *ctx, String8 mnemonic, String8 operands);
void write_fmt_line_no_operands(t_ctx *ctx, String8 mnemonic);

#endif