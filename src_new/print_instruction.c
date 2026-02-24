#include "print_instruction.h"

String8 table_reg_w_zero[8] =
{
    STR8_LIT("al"),
    STR8_LIT("cl"),
    STR8_LIT("dl"),
    STR8_LIT("bl"),
    STR8_LIT("ah"), 
    STR8_LIT("ch"), 
    STR8_LIT("dh"),
    STR8_LIT("bh"),
};

String8 table_reg_w_one[8] =
{
    STR8_LIT("ax"),
    STR8_LIT("cx"),
    STR8_LIT("dx"),
    STR8_LIT("bx"),
    STR8_LIT("sp"), 
    STR8_LIT("bp"), 
    STR8_LIT("si"),
    STR8_LIT("di"),
};

String8 table_mem_address_calc[8] = 
{
    STR8_LIT("bx + si"),
    STR8_LIT("bx + di"), 
    STR8_LIT("bp + si"), 
    STR8_LIT("bp + di"), 
    STR8_LIT("si"),
    STR8_LIT("di"),
    STR8_LIT("bp"),
    STR8_LIT("bx"),
};

String8 table_sreg[4] =
{
    STR8_LIT("es"),
    STR8_LIT("cs"),
    STR8_LIT("ss"),
    STR8_LIT("ds"),
};

typedef String8 (*translateOperand)(Arena *arena, Operand op, u8 W);

String8 translateOpNone(Arena *arena, Operand op, u8 W);
String8 translateOpRegister(Arena *arena, Operand op, u8 W);
String8 translateOpRegisterDx(Arena *arena, Operand op, u8 W);
String8 translateOpRegisterCl(Arena *arena, Operand op, u8 W);
String8 translateOpSegRegister(Arena *arena, Operand op, u8 W);
String8 translateOpImmediate(Arena *arena, Operand op, u8 W);
String8 translateOpMemory(Arena *arena, Operand op, u8 W);
String8 translateOpMemoryDir(Arena *arena, Operand op, u8 W);
String8 translateOpIpRelative(Arena *arena, Operand op, u8 W);

translateOperand translate_table[OP_COUNT] =
{
    [OP_NONE] = translateOpNone,
    [OP_REGISTER] = translateOpRegister,
    [OP_REGISTER_DX] = translateOpRegisterDx,
    [OP_REGISTER_CL] = translateOpRegisterCl,
    [OP_SREG] = translateOpSegRegister,
    [OP_IMMEDIATE] = translateOpImmediate,
    [OP_MEMORY] = translateOpMemory,
    [OP_MEMORY_DIR] = translateOpMemoryDir,
    [OP_IP_RELATIVE] = translateOpIpRelative,
};

String8 translateOpNone(Arena *arena, Operand op, u8 W)
{
    (void)arena;
    (void)op;
    (void)W;
    return (String8){0};
};

String8 translateOpRegister(Arena *arena, Operand op, u8 W)
{
    (void)arena;
    if (W == 0)
    {
        return table_reg_w_zero[op.reg_idx];
    }
    else
    {
        return table_reg_w_one[op.reg_idx];
    }
}

String8 translateOpRegisterDx(Arena *arena, Operand op, u8 W)
{
    (void)W;
    (void)arena;
    return table_reg_w_one[op.reg_idx];
}

String8 translateOpRegisterCl(Arena *arena, Operand op, u8 W)
{
    (void)W;
    (void)arena;
    return table_reg_w_zero[op.reg_idx];
}
String8 translateOpSegRegister(Arena *arena, Operand op, u8 W)
{
    (void)W;
    (void)arena;
    return table_sreg[op.reg_idx];
}

String8 translateOpImmediate(Arena *arena, Operand op, u8 W)
{
    (void)W;
    (void)arena;
    return str8_fmt(arena, STR8_LIT("%u"), op.immediate_val);
}

String8 translateOpMemory(Arena *arena, Operand op, u8 W)
{
    (void)W;
    if (op.mem_disp != 0)
    {
        if (op.mem_disp < 0)
        {
            return str8_fmt(arena, STR8_LIT("[%s - %u]"), table_mem_address_calc[op.mem_base_reg], (u16)-(op.mem_disp));
        }
        else
        {
            return str8_fmt(arena, STR8_LIT("[%s + %u]"), table_mem_address_calc[op.mem_base_reg], (u16)op.mem_disp);
        }
    }
    else
    {
        return str8_fmt(arena, STR8_LIT("[%s]"), table_mem_address_calc[op.mem_base_reg]);
    }
}

String8 translateOpMemoryDir(Arena *arena, Operand op, u8 W)
{
    (void)W;
    return str8_fmt(arena, STR8_LIT("[%u]"), (u16)op.mem_disp);
}

String8 translateOpIpRelative(Arena *arena, Operand op, u8 W)
{
    (void)W;
    return str8_fmt(arena, STR8_LIT("%u"), op.immediate_val);
}


u8 is_shift(String8 mnemonic)
{
    return  (mnemonic.size == 3 && \
            (mnemonic.str[0] == 's' || mnemonic.str[0] == 'r') && \
            (mnemonic.str[2] == 'l' || mnemonic.str[2] == 'r')
            );
}

void print_instruction(Arena *arena, s32 fd, Instruction inst)
{
    OperandType dType = inst.dest.type;
    OperandType sType = inst.src.type;
    String8 dest = translate_table[dType](arena, inst.dest, inst.w_bit);
    String8 src = translate_table[sType](arena, inst.src, inst.w_bit);
    String8 res;
    String8 prefix;
    if (inst.w_bit == 0)
    {
        prefix = STR8_LIT("byte");
    }
    else
    {
        prefix = STR8_LIT("word");
    }
    
    if (dType == OP_NONE && sType == OP_NONE)
    {
        s32 is_prefix =  !str8ncmp(inst.mnemonic, STR8_LIT("rep"), 3) || \
                        !str8ncmp(inst.mnemonic, STR8_LIT("repne"), 5) || \
                        !str8ncmp(inst.mnemonic, STR8_LIT("lock"), 4);
        if (is_prefix)
        {
            res = str8_fmt(arena, STR8_LIT("%s "), inst.mnemonic);
        }
        else
        {
            res = str8_fmt(arena, STR8_LIT("%s"), inst.mnemonic);
        }
    }
    else if (src.size != 0)
    {   
        if ((dType == OP_MEMORY || dType == OP_MEMORY_DIR) && (sType == OP_IMMEDIATE || is_shift(inst.mnemonic)))
        {
            res = str8_fmt(arena, STR8_LIT("%s %s, %s %-5s"), inst.mnemonic, dest, prefix, src);
        }
        else
        {
            res = str8_fmt(arena, STR8_LIT("%s %s, %-5s"), inst.mnemonic, dest, src);
        }
    }
    else
    {
        if (dType == OP_MEMORY || dType == OP_MEMORY_DIR)
        {
            res = str8_fmt(arena, STR8_LIT("%s %s %-5s"), inst.mnemonic, prefix, dest);
        }
        else
        {
            res = str8_fmt(arena, STR8_LIT("%s %-5s"), inst.mnemonic, dest);
        }
    }
    write(fd, res.str, res.size);
}