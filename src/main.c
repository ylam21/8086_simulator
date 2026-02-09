#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>


#define MASK_WIDE 0
#define MASK_LOW 1
#define MASK_HIGH 2

static u8 get_reg_idx_and_set_flag(String8 reg, String8 regs_names[12], u8 *flag);

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

static u16 mask_val(u16 val, u8 flag)
{
    if (flag == MASK_WIDE)
    {
        return val & 0xFFFF;
    }
    else if (flag == MASK_LOW)
    {
        return val & 0x00FF;
    }
    else if (flag == MASK_HIGH)
    {
        return (val >> 8) & 0x00FF;
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
        return (u16)-1;
    }
} 

static void set_reg_value(u16 *reg, u16 value, u8 flag_dest)
{
    if (flag_dest == MASK_WIDE)
    {
        *reg = value;
    }
    else if (flag_dest == MASK_LOW)
    {
        *reg = (*reg & 0xFF00) | (value & 0xFF);
    }
    else if (flag_dest == MASK_HIGH)
    {
        *reg = (*reg & 0x00FF) | ((value & 0xFF) << 8);
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
    }
}

static void modify_dest(String8 mnemonic, u16 *dest_reg, u8 flag_dest, u16 src)
{
    if (str8ncmp(mnemonic, STR8_LIT("mov"), mnemonic.size) == 0)
    {
        set_reg_value(dest_reg, src, flag_dest);
    }
    else if (str8ncmp(mnemonic, STR8_LIT("add"), mnemonic.size) == 0)
    {
        *dest_reg += src;
    }
}

static u8 get_reg_idx_and_set_flag(String8 reg, String8 regs_names[12], u8 *flag)
{
    u8 idx;

    idx = 0;
    while (idx < 4)
    {
        if (str8ncmp(reg, regs_names[idx], 1) == 0)
        {
            u8 c = reg.str[1];
            if (c == 'x')
            {
                *flag = MASK_WIDE; // 16-bit wide
                return idx;
            }
            else if (c == 'l')
            {
                *flag = MASK_LOW; // 8-bit low
                return idx;
            }
            else if (c == 'h')
            {
                *flag = MASK_HIGH; // 8-bit high
                return idx;
            }
        }
        idx += 1;
    }

    while (idx < 12)
    {
        if (str8ncmp(reg, regs_names[idx], reg.size) == 0)
        {
            return idx;
        }
        idx += 1;
    }
    return -1;
}

static s16 get_value_from_src(String8 src, u16 *regs, String8 regs_names[12])
{
    if (src.size == 1)
    {
        return (s16)src.str[0] - '0';
    }
    else if (src.size == 2 && is_alpha(src.str[0]))
    {
        u8 flag = MASK_WIDE;
        u8 idx = get_reg_idx_and_set_flag(src, regs_names, &flag);
        u16 val = mask_val(regs[idx], flag);
        return (s16)val;
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

static void process_line(Arena *arena, u16 *regs, String8 regs_names[12], String8 line, s32 fd)
{
    u16 regs_old[12];
    memcpy(regs_old, regs, 12 * (sizeof(u16)));

    String8 mnemonic, dest, src;
    u64 pos = 0;

    pos += get_mnemonic(line, &mnemonic, pos);
    if (mnemonic.size == 0) return;
    pos += get_operand(line, &dest, pos);
    if (dest.size == 0) return;
    pos += get_operand(line, &src, pos);
    if (src.size == 0) return;

    s16 src_value = get_value_from_src(src, regs, regs_names);

    u8 flag_dest = MASK_WIDE;
    u8 reg_dest_idx = get_reg_idx_and_set_flag(dest, regs_names, &flag_dest);
    if (reg_dest_idx != (u8)-1)
    {
        modify_dest(mnemonic, &regs[reg_dest_idx], flag_dest, src_value);
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

    String8 regs_names[12] =
    {
       STR8_LIT("ax"), // General-purpose registers
       STR8_LIT("bx"), // 'ax' ... 'di'
       STR8_LIT("cx"),
       STR8_LIT("dx"),
       STR8_LIT("sp"),
       STR8_LIT("bp"),
       STR8_LIT("si"),
       STR8_LIT("di"),
       STR8_LIT("es"), // Segment registers
       STR8_LIT("cs"), // 'es' ... 'ds'
       STR8_LIT("ss"),
       STR8_LIT("ds"),
    };
    u16 regs[12] = {0}; // Array where we store the state 12 registers, 16-bit wide each

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
