#include "string_utils.h"

void write_final_regs(Arena *arena, s32 fd, u16 regs[8])
{
    String8 ax = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"ax", .size = 2}, regs[0], regs[0]);
    String8 bx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"bx", .size = 2}, regs[1], regs[1]);
    String8 cx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"cx", .size = 2}, regs[2], regs[2]);
    String8 dx = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"dx", .size = 2}, regs[3], regs[3]);
    String8 sp = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"sp", .size = 2}, regs[4], regs[4]);
    String8 bp = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"bp", .size = 2}, regs[5], regs[5]);
    String8 si = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"si", .size = 2}, regs[6], regs[6]);
    String8 di = str8_fmt(arena,STR8_LIT("%7s: 0x%04x (%d)\n"), (String8){.str = (u8 *)"di", .size = 2}, regs[7], regs[7]);
    String8 result = str8_fmt(arena, STR8_LIT("\nFinal Registers:\n%s%s%s%s%s%s%s%s"), ax, bx, cx, dx, sp, bp, si, di);
    write(fd, result.str, result.size);
}

void write_line(Arena *arena, s32 fd, String8 line, String8 dest, u16 reg_start, u16 regs_end)
{
    String8 result = str8_fmt(arena,STR8_LIT("%-15s; %s: 0x%04x->0x%04x\n"), line, dest, reg_start, regs_end);
    write(fd, result.str, result.size);
}
