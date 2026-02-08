#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static u8 get_reg_idx(u8 *reg, u8 *regs_names[8]);

static u64 get_mnemonic(String8 line, String8 *mnemonic, u64 pos)
{   
    u64 offset = 0;
    while ((offset + pos) < line.size && is_whitespace(line.str[offset + pos]))
    {
        offset += 1;
    }
    
    mnemonic->str = line.str + pos + offset;
    
    u64 len = 0;
    while ((offset + pos) < line.size && is_alpha(line.str[offset + pos])) 
    {
        len += 1;
        offset += 1;
    }

    mnemonic->size = len;

    return offset;
}


static u64 get_operand(String8 line, String8 *operand, u64 pos)
{
    u64 offset = 0;
    while ((offset + pos) < line.size && is_whitespace(line.str[offset + pos]))
    {
        offset += 1;
    }
    
    operand->str = line.str + pos + offset;
    
    u64 len = 0;
    while ((offset + pos) < line.size && is_alnum(line.str[offset + pos])) 
    {
        len += 1;
        offset += 1;
    }

    operand->size = len;

    return offset;
}


static s64 get_value_from_reg(String8 reg, u16 regs[8], u8 *regs_names[8])
{
    u8 idx;

    idx = 0;
    while (idx < 8)
    {
        if (strncmp((char*)reg.str, (char *)regs_names[idx], 2) == 0)
        {
            return (regs[idx]);
        }
        idx += 1;
    }

    return 0;
}

static void modify_dest(u16 *regs, u8 *regs_names[8], String8 dest, s64 src_value)
{
    u8 idx = get_reg_idx(dest.str, regs_names);
    regs[idx] = src_value;
}

static u8 get_reg_idx(u8 *reg, u8 *regs_names[8])
{
    u8 idx;

    idx = 0;
    while (idx < 8)
    {
        if (strncmp((char *)reg, (char *)regs_names[idx], 2) == 0)
        {
            return idx;
        }
        idx += 1;
    }
    return 0;
}

static void write_line(Arena *arena, s32 fd, String8 line, String8 dest, u16 reg_start, u16 regs_end)
{
    String8 result = str8_fmt(arena,(u8 *)"%s ; %s:%x->%x\n", line, dest, reg_start, regs_end);
    write(fd, result.str, result.size);
}

static void process_line(Arena *arena, u16 *regs, String8 line, s32 fd)
{
    u8 *regs_names[8] =
    {
        (u8*)"ax", (u8*)"bx",(u8*) "cx",(u8*) "dx",
        (u8*)"sp",(u8*) "bp",(u8*) "si",(u8*) "di"
    };

    u16 regs_old[8];
    memcpy(regs_old, regs, 8 * (sizeof(u16)));

    String8 mnemonic, dest, src;
    u64 pos = 0;

    pos += get_mnemonic(line, &mnemonic, pos); // not needed for the first few exercices
    if (mnemonic.size == 0) return;
    pos += get_operand(line, &dest, pos);
    while (pos < line.size && is_whitespace(line.str[pos]))
    {
        pos += 1;
    }
    pos += 1;
    pos += get_operand(line, &src, pos);

    s64 src_value;
    if (is_num(src.str[0]))
    {
        src_value = string8tos64(src);
    }
    else
    {
        src_value = get_value_from_reg(src, regs_old, regs_names);
    }

    u8 reg_dest_idx = get_reg_idx(dest.str, regs_names);
    modify_dest(regs, regs_names, dest, src_value);
    write_line(arena, fd, line, dest, regs_old[reg_dest_idx], regs[reg_dest_idx]);
}

static String8 extract_line(String8 buffer, u64 pos)
{
    u64 len = 0;
    while ((pos + len) < buffer.size)
    {
        if (buffer.str[pos + len] == '\n')    
        {
            break;
        }
        len++;
    }
    return (String8){ .size = len, .str = buffer.str + pos};
}

static void write_final_regs(Arena *arena, s32 fd, u16 regs[8])
{
    String8 ax = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"ax", .size = 2}, regs[0], regs[0]);
    String8 bx = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"bx", .size = 2}, regs[1], regs[1]);
    String8 cx = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"cx", .size = 2}, regs[2], regs[2]);
    String8 dx = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"dx", .size = 2}, regs[3], regs[3]);
    String8 sp = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"sp", .size = 2}, regs[4], regs[4]);
    String8 bp = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"bp", .size = 2}, regs[5], regs[5]);
    String8 si = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"si", .size = 2}, regs[6], regs[6]);
    String8 di = str8_fmt(arena,(u8 *) "%7s: 0x%04x (%d)\n", (String8){.str = (u8 *)"di", .size = 2}, regs[7], regs[7]);
    String8 result = str8_fmt(arena, (u8 *)"\nFinal Registers:\n%s%s%s%s%s%s%s%s", ax, bx, cx, dx, sp, bp, si, di);
    write(fd, result.str, result.size);
}

static void simulate_8086(Arena *arena, String8 buffer, s32 fd)
{
    u16 regs[8] = {0}; // Array where we store 8 registers, 16-bit wide each
    u64 pos = 0;

    pos += strlen("bits 16");
    while (pos < buffer.size && is_whitespace(buffer.str[pos]))
    {
        pos += 1;
    }

    while (pos < buffer.size)
    {
        String8 line = extract_line(buffer, pos);
        process_line(arena, regs, line, fd);
        pos += line.size;
        if (pos < buffer.size && buffer.str[pos] == '\n')
        {
            pos += 1;
        }
        arena_reset(arena);
    }
    write_final_regs(arena, fd, regs);
}

static String8 read_file(Arena *arena, s32 fd)
{
    u64 len = (u64)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    u8 *data = arena_push(arena, len);
    if (!data)
    {
        return (String8){0};
    }
    
    s64 bytes_read = read(fd, data, len);
    if (bytes_read != (s64)len)
    {
        fprintf(stderr, "Error: Read unexpected amount of bytes\n");
        return (String8){0};
    }

    return (String8){ .str = data, .size = len};
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename.asm>\n", PROGRAM_PATH);
        return (EXIT_FAILURE);
    }

    
    u8 *filename_in = (u8 *)argv[1];
    s32 fd_in = open((char *)filename_in, O_RDONLY);
    if (fd_in == -1)
    {
        fprintf(stderr, "Error: Could not open %s\n", filename_in);
        return (EXIT_FAILURE);
    }
    
    u8 *filename_out = (u8 *)"out.txt";
    s32 fd_out = open((char *)filename_out, O_CREAT | O_TRUNC | O_WRONLY, 0777);
    if (fd_out == -1)
    {
        fprintf(stderr, "Error: Could not open %s\n", filename_out);
        close(fd_in);
        return (EXIT_FAILURE);
    }

    Arena *life_arena = arena_create(1024 * 1024);
    if (!life_arena)
    {
        close(fd_in);
        close(fd_out);
        return (EXIT_FAILURE);
    }
    
    String8 buffer = read_file(life_arena, fd_in);
    if (buffer.str)
    {
        simulate_8086(life_arena, buffer, fd_out);
        arena_destroy(life_arena);
        close(fd_in);
        close(fd_out);
        return (EXIT_SUCCESS);
    }

    close(fd_in);
    close(fd_out);
    arena_destroy(life_arena);
    return (EXIT_FAILURE);
}
