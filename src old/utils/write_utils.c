#include "string_utils.h"
#include "../simulate_8086.h"

String8 state_of_flags(Arena *arena, u16 reg_old, u16 reg_new);
String8 create_state_of_flag_reg(Arena *arena, u16 reg);

void write_final_regs(Arena *arena, s32 fd, u16 regs[14])
{
    String8 ax = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"ax", .size = 2}, regs[0], regs[0]);
    String8 bx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"bx", .size = 2}, regs[1], regs[1]);
    String8 cx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"cx", .size = 2}, regs[2], regs[2]);
    String8 dx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"dx", .size = 2}, regs[3], regs[3]);
    String8 sp = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"sp", .size = 2}, regs[4], regs[4]);
    String8 bp = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"bp", .size = 2}, regs[5], regs[5]);
    String8 si = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"si", .size = 2}, regs[6], regs[6]);
    String8 di = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"di", .size = 2}, regs[7], regs[7]);
    String8 es = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"es", .size = 2}, regs[8], regs[8]);
    String8 cs = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"cs", .size = 2}, regs[9], regs[9]);
    String8 ss = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"ss", .size = 2}, regs[10], regs[10]);
    String8 ds = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"ds", .size = 2}, regs[11], regs[11]);
    String8 flags = create_state_of_flag_reg(arena, regs[13]);
    String8 flag_field = str8_fmt(arena, STR8_LIT("%6s%s]"), STR8_LIT("["),flags);
    String8 result = str8_fmt(arena, STR8_LIT("\nFinal Registers:\n%s%s%s%s%s%s%s%s%s%s%s%s%s"), ax, bx, cx, dx, sp, bp, si, di, es, cs, ss, ds, flag_field);
    write(fd, result.str, result.size);
}

void write_line(Arena *arena, s32 fd, String8 line, String8 dest, u8 reg_dest_idx, u16 regs_old[14], u16 regs_new[14])
{
    String8 flags = state_of_flags(arena, regs_old[13], regs_new[13]);
    String8 result = str8_fmt(arena,STR8_LIT("%-15s; %s: 0x%04x->0x%04x %s\n"), line, dest, regs_old[reg_dest_idx], regs_new[reg_dest_idx], flags);
    write(fd, result.str, result.size);
}

static const u8 flag_map[9] = { 
    POS_OF, POS_DF, POS_IF, POS_TF, POS_SF, POS_ZF, POS_AF, POS_PF, POS_CF 
};

String8 create_state_of_flag_reg(Arena *arena, u16 reg)
{ 
    String8 flags = STR8_LIT("ODITSZAPC");

    u8 *str = arena_push(arena, flags.size);
    if (!str) return (String8){0};

    memcpy(str, flags.str, flags.size);

    u8 pos = 0;
    while (pos < flags.size)
    {
        u8 bit_pos = flag_map[pos];
        u8 res = (reg >> bit_pos) & 1;
        if (res == 0)
        {
            str[pos] = CHAR_SPACE;
        }
        pos += 1;
    }
 
    return (String8){ .size = flags.size, .str = str};
}


String8 state_of_flags(Arena *arena, u16 reg_old, u16 reg_new)
{
    String8 state_old = create_state_of_flag_reg(arena, reg_old);
    String8 state_new = create_state_of_flag_reg(arena, reg_new);
    return str8_fmt(arena, STR8_LIT("[%s]->[%s]"), state_old, state_new);
}