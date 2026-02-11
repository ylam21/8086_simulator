#include "io_utils.h"

void write_fmt_line_no_operands(t_ctx *ctx, u8 *mnemonic)
{
    u8 *line = strjoin_fmt(ctx->a, "%-7s\n", mnemonic);
    write(ctx->fd, line, strlen(line));
}

void write_fmt_line(t_ctx *ctx, u8 *mnemonic, u8 *operands)
{
    u8 *line = strjoin_fmt(ctx->a, "%-7s %s\n", mnemonic, operands);
    write(ctx->fd, line, strlen(line));
}

