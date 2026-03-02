#ifndef EXECUTE_INSTRUCTION_H
#define EXECUTE_INSTRUCTION_H

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

#define INST_MOV STR8_LIT("mov")
#define INST_ADD STR8_LIT("add")
#define INST_SUB STR8_LIT("sub")
#define INST_CMP STR8_LIT("cmp")
#define INST_JNZ STR8_LIT("jnz")
#define INST_LOOP STR8_LIT("loop")

typedef u16 (*calcMemoryAddressFunc)(u16 *regs);

void execute_instruction(Arena *arena, s32 fd, Cpu *cpu, Instruction inst, u16 *regsStateOld, t_ctx *ctx);
u16 calc_memory_address(Operand op, Cpu *cpu);
void print_final_regs(Arena *arena, s32 fd, u16 *regs);
u16 calc_bx_plus_si(u16 *regs);
u16 calc_bx_plus_di(u16 *regs);
u16 calc_bp_plus_si(u16 *regs);
u16 calc_bp_plus_di(u16 *regs);
u16 calc_si(u16 *regs);
u16 calc_di(u16 *regs);
u16 calc_bp(u16 *regs);
u16 calc_bx(u16 *regs);
void modify_flag_reg(u16 *reg, u32 res, u8 mask_dest);
u8 decode_final_reg_idx_from_reg(Operand reg, u8 W);
u8 decode_final_reg_idx_from_sreg(Operand sreg);
u16 masked_u16(u16 value, u8 mask);
u8 decode_mask_from_reg(Operand reg, u8 W);
void modifyDest(u16 *destPtr, u16 val, u8 mask, OperandType dType);
String8 state_of_flags(Arena *arena, u16 reg_old, u16 reg_new);
String8 create_state_of_flag_reg(Arena *arena, u16 reg);
void mod_SF(u16 *reg, u32 res, u8 mask_dest);
void mod_PF(u16 *reg, u32 res);
void mod_ZF(u16 *reg, u32 res);

#endif