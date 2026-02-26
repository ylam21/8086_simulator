#include "execute_instruction.h"
#include "utils/io_utils.h"
#include <string.h>

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
    STR8_LIT(""),   // flags register
};

void print_final_regs(Arena *arena, s32 fd, u16 *regs)
{
    String8 header = STR8_LIT("\nFinal Registers:\n");
    write(fd, header.str, header.size);

    String8 ax = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[0], regs[0], regs[0]);
    String8 bx = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[3], regs[3], regs[3]);
    String8 cx = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[1], regs[1], regs[1]);
    String8 dx = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[2], regs[2], regs[2]);
    write(fd, ax.str, ax.size);
    write(fd, bx.str, bx.size);
    write(fd, cx.str, cx.size);
    write(fd, dx.str, dx.size);

    u8 pos = 4;
    while (pos < 12)
    {
        String8 line = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[pos], regs[pos], regs[pos]);
        write(fd, line.str, line.size);
        pos += 1;
    }
    String8 flags = create_state_of_flag_reg(arena, regs[13]);
    String8 flag_field = str8_fmt(arena, STR8_LIT("%13s%s]"), STR8_LIT("flags: ["),flags);
    write(fd, flag_field.str, flag_field.size);
}

static void set_reg_value(u16 *reg, u16 val, u8 mask)
{
    if (mask == MASK_WIDE)
    {
        *reg = val;
    }
    else if (mask == MASK_LOW)
    {
        *reg = (*reg & 0xFF00) | (val & 0xFF);
    }
    else if (mask == MASK_HIGH)
    {
        *reg = (*reg & 0x00FF) | ((val & 0xFF) << 8);
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
    }
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

#define INST_MOV STR8_LIT("mov")

void execute_instruction(Arena *arena, s32 fd, Cpu cpu, Instruction inst)
{
    OperandType sType = inst.src.type;
    OperandType dType = inst.dest.type;
    u16 regs_state_old[14] = {0};
    memcpy(regs_state_old, cpu.regs, 14 * sizeof(u16));
    u8 reg_dest_idx;
    if (dType == OP_REGISTER || dType == OP_REGISTER_CL || dType == OP_REGISTER_DX)
    {
        reg_dest_idx = inst.dest.reg_idx;
    }
    else if (dType == OP_SREG)
    {
        reg_dest_idx = inst.dest.reg_idx + 8; 
    }
    
    if (!str8ncmp(inst.mnemonic, INST_MOV, INST_MOV.size))
    {   
        u8 mask;
        u16 val;
        if (sType == OP_REGISTER || sType == OP_REGISTER_CL || sType == OP_REGISTER_DX)
        {
            mask = decode_mask_from_reg(inst.src, inst.w_bit);
            val = cpu.regs[inst.src.reg_idx];
        }
        else if (sType == OP_IMMEDIATE)
        {
            mask = decode_mask_from_reg(inst.src, inst.w_bit);
            val = inst.src.immediate_val;
        }
        set_reg_value(&cpu.regs[reg_dest_idx], val, mask);
    }
    
    u16 *regs_state_new = cpu.regs;
    String8 flags = state_of_flags(arena, regs_state_old[13], regs_state_new[13]);
    String8 res = str8_fmt(arena, STR8_LIT("%c; %s: 0x%04x->0x%04X %s"), CHAR_SPACE, regs_names[reg_dest_idx], regs_state_old[reg_dest_idx], regs_state_new[reg_dest_idx], flags);
    write(fd, res.str, res.size);
}
