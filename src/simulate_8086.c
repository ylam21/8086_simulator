#include "simulate_8086.h"

static u8 get_reg_idx_and_set_flag(String8 reg, String8 regs_names[14], u8 *flag);

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
    
    if (line.str[pos + offset] == '-' || line.str[pos + offset] == '+')
    {
        offset += 1;
    }

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

static void set_reg_value(u16 *reg, u16 val, u8 flag_dest)
{
    if (flag_dest == MASK_WIDE)
    {
        *reg = val;
    }
    else if (flag_dest == MASK_LOW)
    {
        *reg = (*reg & 0xFF00) | (val & 0xFF);
    }
    else if (flag_dest == MASK_HIGH)
    {
        *reg = (*reg & 0x00FF) | ((val & 0xFF) << 8);
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
    }
}


static void set_zf(u16 *reg, u16 val)
{
    if (val)
    {
        *reg = *reg & ~(1 << POS_ZF);
    }
    else
    {
        // val is equal to zero
        *reg = *reg | (1 << POS_ZF);
    }
}

static void set_sf(u16 *reg, u16 val)
{
    if ((val >> 15) & 1)
    {
        // val has the MSB equal to 1
        *reg = *reg | (1 << POS_SF);
    }
    else
    {
        *reg = *reg & ~(1 << POS_SF);
    }
}

static void modify_dest(u16 *flags_reg, String8 mnemonic, u16 *dest_reg, u8 flag_dest, u16 src)
{
    if (str8ncmp(mnemonic, STR8_LIT("mov"), mnemonic.size) == 0)
    {
        set_reg_value(dest_reg, src, flag_dest);
    }
    else if (str8ncmp(mnemonic, STR8_LIT("add"), mnemonic.size) == 0)
    {
        u16 result = *dest_reg + src;
        set_zf(flags_reg, result);
        set_sf(flags_reg, result);
        set_reg_value(dest_reg, result, flag_dest);
    }
    else if (str8ncmp(mnemonic, STR8_LIT("cmp"), mnemonic.size) == 0)
    {
        u16 result = *dest_reg - src;
        set_zf(flags_reg, result);
        set_sf(flags_reg, result);
    } 
    else if (str8ncmp(mnemonic, STR8_LIT("sub"), mnemonic.size) == 0)
    {
        u16 result = *dest_reg - src;
        set_zf(flags_reg, result);
        set_sf(flags_reg, result);
        set_reg_value(dest_reg, result, flag_dest);
    }
}

static u8 get_reg_idx_and_set_flag(String8 reg, String8 regs_names[14], u8 *flag)
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

static s16 get_value_from_src(String8 src, u16 *regs, String8 regs_names[14])
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

static void process_line(Arena *arena, u16 *regs, String8 regs_names[14], String8 line, s32 fd)
{
    u16 regs_old[14];
    memcpy(regs_old, regs, 14 * (sizeof(u16)));

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
        modify_dest(&regs[13], mnemonic, &regs[reg_dest_idx], flag_dest, src_value);
        write_line(arena, fd, line, dest, reg_dest_idx, regs_old, regs);
    }
    else
    {
        fprintf(stderr, "Error: get_reg_idx returns garbage\n");
    }
}

static String8 extract_line(String8 data, u64 pos)
{
    u64 len = 0;
    while ((pos + len) < data.size && data.str[pos + len] != '\n' && data.str[pos + len] != '\r')
    {
        len += 1;
    }
    return (String8){ .size = len, .str = data.str + pos};
}

void simulate_8086(String8 data, s32 fd)
{
    Arena *scratch_arena = arena_create(1024 * 1024);
    if (!scratch_arena) return;

    u16 regs[14] = {0}; // Array where we store the state of all 14 registers, 16-bit wide each
    String8 regs_names[14] =
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
       STR8_LIT("ip"), // Instruction pointer register
       STR8_LIT(""),   // flags register
    };

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