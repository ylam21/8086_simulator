#include "io_utils.h"

void write_fmt_line_no_operands(t_ctx *ctx, String8 mnemonic)
{
    String8 line = str8_fmt(ctx->arena, STR8_LIT("%-7s\n"), mnemonic);
    write(ctx->fd, line.str, line.size);
}

void write_fmt_line(t_ctx *ctx, String8 mnemonic, String8 operands)
{
    String8 line = str8_fmt(ctx->arena, STR8_LIT("%-7s %s"), mnemonic, operands);
    write(ctx->fd, line.str, line.size);
}


void write_fmt_sim_line(t_ctx *ctx, String8 dest, u16 reg_old, u16 reg_new)
{
    String8 result = str8_fmt(ctx->arena, STR8_LIT(" ; %s: 0x%04x->0x%04x %s"), dest, reg_old, reg_new);
    write(ctx->fd, result.str, result.size);
}

static String8 decode_reg_name(u8 idx)
{
    if (idx < 8)
    {
        return table_reg_w_one[idx];
    }
    else
    {
        return table_sreg[idx - 8];
    }
}

void write_final_regs(Arena *arena, s32 fd, u16 regs[14])
{
    String8 final = STR8_LIT("\nFinal Registers:\n");
    write(fd, final.str, final.size);

    u8 i = 0;
    while (i < 12)
    {
        String8 reg_name = decode_reg_name(i);
        u16 reg_val = regs[i];
        String8 reg = str8_fmt(arena, STR8_LIT("%7s: 0x%04x (%d)\n"), reg_name, reg_val, reg_val);
        write(fd, reg.str, reg.size);
        i += 1;
    }
}