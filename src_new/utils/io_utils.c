#include "io_utils.h"

void write_fmt_line_no_operands(t_ctx *ctx, String8 mnemonic)
{
    String8 line = str8_fmt(ctx->arena, STR8_LIT("%-7s\n"), mnemonic);
    write(ctx->fd, line.str, line.size);
}

void write_fmt_line(t_ctx *ctx, String8 mnemonic, String8 operands)
{
    String8 line = str8_fmt(ctx->arena, STR8_LIT("%-7s %s\n"), mnemonic, operands);
    write(ctx->fd, line.str, line.size);
}

