#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static u8 get_reg_idx(String8 reg, String8 regs_names[8]);

static u64 get_mnemonic(String8 line, String8 *mnemonic, u64 pos)
{   
    u64 offset = 0;
    while ((offset + pos) < line.size && is_whitespace(line.str[offset + pos]))
    {
        offset += 1;
    }
    
    mnemonic->str = line.str + pos + offset;
    
    u64 len = 0;
    while ((offset + pos + len) < line.size && is_alpha(line.str[offset + pos + len])) 
    {
        len += 1;
    }

    mnemonic->size = len;

    return offset + len;
}


static u64 get_operand(String8 line, String8 *operand, u64 pos)
{
    u64 offset = 0;
    while ((offset + pos) < line.size && (is_whitespace(line.str[offset + pos]) || line.str[offset + pos] == ','))
    {
        offset += 1;
    }
    
    operand->str = line.str + pos + offset;
    
    u64 len = 0;
    while ((offset + pos + len) < line.size && is_alnum(line.str[offset + pos + len])) 
    {
        len += 1;
    }

    operand->size = len;

    return offset + len;
}

static u8 str8ncmp(String8 s1, String8 s2, u64 n)
{
    u8 pos = 0;
    while (pos < s1.size && pos < s2.size && pos < n)
    {
        if (s1.str[pos] != s2.str[pos])
        {
            return 2;
        }
        pos += 1;
    }
    return 0;
}

static void modify_dest(String8 mnemonic, u16 *regs, String8 regs_names[8], String8 dest, s64 src_value)
{
    u8 dest_idx = get_reg_idx(dest, regs_names);

    if (str8ncmp(mnemonic, STR8_LIT("mov"), mnemonic.size) == 0)
    {
        regs[dest_idx] = src_value;
    }
    else if (str8ncmp(mnemonic, STR8_LIT("add"), mnemonic.size) == 0)
    {
        regs[dest_idx] += src_value;
    }
}

static u8 get_reg_idx(String8 reg, String8 regs_names[8])
{
    u8 idx;

    idx = 0;
    while (idx < 8)
    {
        if (str8ncmp(reg, regs_names[idx], reg.size) == 0)
        {
            return idx;
        }
        idx += 1;
    }
    return -1;
}

static s16 get_value_from_src(String8 src, u16 *regs, String8 regs_names[8])
{
    if (src.size == 1)
    {
        return (s16)src.str[0] - '0';
    }
    else if (src.size == 2 && is_alpha(src.str[0]))
    {
        u8 idx = get_reg_idx(src, regs_names);
        return (s16)regs[idx];
    }
    else if (src.size > 2 && src.str[1] == 'x')
    {   
        return string8_hex_to_s16(src);
    }
    else
    {
        return string8_decimal_to_s16(src);
    }
}

static void process_line(Arena *arena, u16 *regs, String8 regs_names[8], String8 line, s32 fd)
{
    u16 regs_old[8];
    memcpy(regs_old, regs, 8 * (sizeof(u16)));

    String8 mnemonic, dest, src;
    u64 pos = 0;

    pos += get_mnemonic(line, &mnemonic, pos);
    if (mnemonic.size == 0) return;
    pos += get_operand(line, &dest, pos);
    if (dest.size == 0) return;
    pos += get_operand(line, &src, pos);
    if (src.size == 0) return;

    s16 src_value = get_value_from_src(src, regs, regs_names);

    u8 reg_dest_idx = get_reg_idx(dest, regs_names);
    if (reg_dest_idx != (u8)-1)
    {
        modify_dest(mnemonic, regs, regs_names, dest, src_value);
        write_line(arena, fd, line, dest, regs_old[reg_dest_idx], regs[reg_dest_idx]);
    }
    else
    {
        fprintf(stderr, "Error: get_reg_idx returns garbage\n");
    }
}

static String8 extract_line(String8 data, u64 pos)
{
    u64 len = 0;
    while ((pos + len) < data.size && data.str[pos + len] != '\n')
    {
        len += 1;
    }
    return (String8){ .size = len, .str = data.str + pos};
}

static void simulate_8086(String8 data, s32 fd)
{
    Arena *scratch_arena = arena_create(1024 * 1024);
    if (!scratch_arena) return;

    String8 regs_names[8] =
    {
       STR8_LIT("ax"),
       STR8_LIT("bx"),
       STR8_LIT("cx"),
       STR8_LIT("dx"),
       STR8_LIT("sp"),
       STR8_LIT("bp"),
       STR8_LIT("si"),
       STR8_LIT("di"),
    };
    u16 regs[8] = {0}; // Array where we store 8 registers, 16-bit wide each

    u64 pos = 0;

    if (data.size >= 7 && str8ncmp(data, STR8_LIT("bits 16"), 7) == 0)
    {
        pos += 7;
    }

    while (pos < data.size && is_whitespace(data.str[pos]))
    {
        pos += 1;
    }
    while (pos < data.size)
    {
        String8 line = extract_line(data, pos);
        if (line.size == 0)
        {
            pos += 1;
        }
        else
        {
            process_line(scratch_arena, regs, regs_names, line, fd);
            pos += line.size;
        }
        arena_reset(scratch_arena);
    }
    write_final_regs(scratch_arena, fd, regs);
    arena_destroy(scratch_arena);
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
        simulate_8086(buffer, fd_out);
        fprintf(stdout, "Output written to: \"%s\"\n", filename_out);
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
