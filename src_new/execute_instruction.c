#include "execute_instruction.h"
#include "decoder/opcodes.h"
#include "decoder/decoder.h"
#include "utils/io_utils.h"
#include <string.h>
#include <stdbool.h>

#define MASK_WIDE 0
#define MASK_LOW 1
#define MASK_HIGH 2

#define POS_CF 0  // carry status flag ; 'POS' stands for position in its register
#define POS_PF 2  // parity status flag
#define POS_AF 4  // auxiliary status flag
#define POS_ZF 6  // zero status flag
#define POS_SF 7  // sign status flag
#define POS_TF 8  // overflow status flag
#define POS_IF 9  // interrupt-enable control flag
#define POS_DF 10 // direction control flag
#define POS_OF 11 // trap control flag

static const u8 flag_map[9] = { 
    POS_OF, POS_DF, POS_IF, POS_TF, POS_SF, POS_ZF, POS_AF, POS_PF, POS_CF 
};

static void mod_ZF(u16 *reg, u32 res)
{
    if (res == 0)
    {
        *reg |= (1 << POS_ZF);
    }
    else
    {
        *reg &= ~(1 << POS_ZF);
    }
}

static void mod_PF(u16 *reg, u32 res)
{
    u8 cnt = __builtin_popcount(res & 0xFF);

    if (!(cnt & 1))
    {
        *reg |= (1 << POS_PF);
    }
    else
    {
        *reg &= ~(1 << POS_PF);
    }
}

static void mod_SF(u16 *reg, u32 res, u8 mask_dest)
{
    u8 width_bit;
    if (mask_dest == MASK_WIDE || mask_dest == MASK_HIGH)
    {
        width_bit = 15;
    }
    else
    {
        width_bit = 7;
    }

    if ((res >> width_bit) & 1)
    {
        *reg |= (1 << POS_SF);
    }
    else
    {
        *reg &= ~(1 << POS_SF);
    }
}

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

String8 regs_names[14] =
{
    STR8_LIT("ax"), // General-purpose registers
    STR8_LIT("cx"), // 'ax' ... 'di'
    STR8_LIT("dx"),
    STR8_LIT("bx"),
    STR8_LIT("sp"),
    STR8_LIT("bp"),
    STR8_LIT("si"),
    STR8_LIT("di"),
    STR8_LIT("es"), // Segment registers
    STR8_LIT("cs"), // 'es' ... 'ds'
    STR8_LIT("ss"),
    STR8_LIT("ds"),
    STR8_LIT("ip"), // Instruction pointer register
    STR8_LIT("flags"),   // flags register
};

void print_final_regs(Arena *arena, s32 fd, u16 *regs)
{
    String8 header = STR8_LIT("\nFinal Registers:\n");
    write(fd, header.str, header.size);

    u8 idx_order[13] = {0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    u8 i = 0;
    while (i < 13)
    {
        u8 pos = idx_order[i];
        u16 res = regs[pos];
        if (res)
        {
            String8 line = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[pos], regs[pos], regs[pos]);
            write(fd, line.str, line.size);
        }
        i += 1;
    }
    String8 flags = create_state_of_flag_reg(arena, regs[FLAGS_IDX]);
    String8 flag_field = str8_fmt(arena, STR8_LIT("%10s: [%s]"), regs_names[FLAGS_IDX], flags);
    write(fd, flag_field.str, flag_field.size);
}

static void modify_reg(u16 *reg, u16 val, u8 mask)
{
    if (mask == MASK_WIDE)
    {
        *reg = val;
    }
    else if (mask == MASK_LOW)
    {
        *reg = (*reg & 0xFF00) | val;
    }
    else if (mask == MASK_HIGH)
    {
        *reg = (*reg & 0x00FF) | (val << 8);
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
    }
}

u8 decode_mask_from_sreg(void)
{
    return MASK_WIDE;
}

u8 decode_mask_from_reg(Operand reg, u8 W)
{
    if (W == 0)
    {
        if (reg.reg_idx < 4)
        {
            return MASK_LOW;
        }
        else
        {
            return MASK_HIGH;
        }
    }
    else
    {
        return MASK_WIDE;
    }
}

u16 masked_u16(u16 value, u8 mask)
{
    if (mask == MASK_WIDE)
    {
        return value;
    }
    else if (mask == MASK_LOW)
    {
        return (value & 0x00FF);
    }
    else if (mask == MASK_HIGH)
    {
        return (value & 0xFF00) >> 8;
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
        return 0;
    }
}

u8 decode_final_reg_idx_from_sreg(Operand sreg)
{
    return sreg.reg_idx + 8;
}

u8 decode_final_reg_idx_from_reg(Operand reg, u8 W)
{
    if (W == 0)
    {
        return (reg.reg_idx % 4);
    }
    else
    {
        return reg.reg_idx;
    }
}

static void modify_flag_reg(u16 *reg, u32 res, u8 mask_dest)
{
    mod_ZF(reg, res);
    mod_PF(reg, res);
    mod_SF(reg, res, mask_dest);
}

#define INST_MOV STR8_LIT("mov")
#define INST_ADD STR8_LIT("add")
#define INST_SUB STR8_LIT("sub")
#define INST_CMP STR8_LIT("cmp")
#define INST_JNZ STR8_LIT("jnz")

void execute_instruction(Arena *arena, s32 fd, Cpu cpu, Instruction inst, u16 *regsStateOld, t_ctx *ctx)
{
    OperandType sType = inst.src.type;
    OperandType dType = inst.dest.type;

    u8 dest_reg_idx;
    u8 mask_dest;
    if (dType == OP_REGISTER || dType == OP_REGISTER_CL || dType == OP_REGISTER_DX)
    {
        dest_reg_idx = decode_final_reg_idx_from_reg(inst.dest, inst.w_bit);
        mask_dest = decode_mask_from_reg(inst.dest, inst.w_bit);
    }
    else if (dType == OP_SREG)
    {
        dest_reg_idx = decode_final_reg_idx_from_sreg(inst.dest); 
        mask_dest = decode_mask_from_sreg();
    }

    u16* destRegPtr = &cpu.regs[dest_reg_idx];
    u16* ipRegPtr = &cpu.regs[IP_IDX];
    u16* flagRegPtr = &cpu.regs[FLAGS_IDX];
    u8 src_reg_idx;
    u8 mask_src;
    u16 src_val;
    if (sType == OP_REGISTER || sType == OP_REGISTER_CL || sType == OP_REGISTER_DX)
    {
        src_reg_idx = decode_final_reg_idx_from_reg(inst.src, inst.w_bit);
        mask_src = decode_mask_from_reg(inst.src, inst.w_bit);
        src_val = masked_u16(cpu.regs[src_reg_idx], mask_src); // val is the masked 'src'
    }
    else if (sType == OP_SREG)
    {
        src_reg_idx = decode_final_reg_idx_from_sreg(inst.src);
        mask_src = decode_mask_from_sreg();
        src_val = masked_u16(cpu.regs[src_reg_idx], mask_src);
    }
    else if (sType == OP_IMMEDIATE)
    {
        src_val = inst.src.immediate_val;
    }
    
    u8 dont_print_flags = false;
    u8 dont_print_regs = false;
    u16 new_val;
    if (!str8ncmp(inst.mnemonic, INST_MOV, INST_MOV.size))
    {   
        modify_reg(destRegPtr, src_val, mask_dest);
        dont_print_flags = true;
    }
    else if (!str8ncmp(inst.mnemonic, INST_ADD, INST_ADD.size))
    {
        u32 res = masked_u16(*destRegPtr, mask_dest) + src_val;
        new_val = (u16)res;
        modify_flag_reg(flagRegPtr, new_val, mask_dest);
        modify_reg(destRegPtr, new_val, mask_dest);
    }
    else if (!str8ncmp(inst.mnemonic, INST_SUB, INST_SUB.size))
    {
        u32 res = masked_u16(*destRegPtr, mask_dest) - src_val;
        new_val = (u16)res;
        modify_flag_reg(flagRegPtr, new_val, mask_dest);
        modify_reg(destRegPtr, new_val, mask_dest);
    }
    else if (!str8ncmp(inst.mnemonic, INST_CMP, INST_CMP.size))
    {
        u32 res = masked_u16(*destRegPtr, mask_dest) - src_val;
        new_val = (u16)res;
        modify_flag_reg(flagRegPtr, new_val, mask_dest);
    }
    else if (!str8ncmp(inst.mnemonic, INST_JNZ, INST_JNZ.size))
    {
        if (!((*flagRegPtr >> POS_ZF) & 1))
        {
            ctx->ip = inst.dest.immediate_val;
            modify_reg(ipRegPtr, ctx->ip, MASK_WIDE);
        }
        dont_print_flags = true;
        dont_print_regs = true;
    }
    
    u16 *regsStateNew = cpu.regs;
    String8 dest_reg_name = regs_names[dest_reg_idx];
    String8 flags;
    String8 ip = str8_fmt(arena, STR8_LIT("ip: 0x%04x->0x%04x"), regsStateOld[IP_IDX], regsStateNew[IP_IDX]);
    if (dont_print_flags)
    {
        flags = (String8){0};
    }
    else
    {
        flags = state_of_flags(arena, regsStateOld[FLAGS_IDX], regsStateNew[FLAGS_IDX]);
    }
    String8 res;
    if (dont_print_regs)
    {
        res = str8_fmt(arena, STR8_LIT("%c; %18c %s %s"), CHAR_SPACE, CHAR_SPACE, ip, flags);
    }
    else
    {
        res = str8_fmt(arena, STR8_LIT("%c; %s: 0x%04x->0x%04x %s %s"), CHAR_SPACE, dest_reg_name, regsStateOld[dest_reg_idx], regsStateNew[dest_reg_idx], ip, flags);
    }
    write(fd, res.str, res.size);
}
