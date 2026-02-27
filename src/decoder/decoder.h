#ifndef DECODER_H
#define DECODER_H

#include "../common.h"

typedef struct t_ctx t_ctx;
#define GET_MOD(b) (((b) >> 6) & 0x3)
#define GET_REG(b) (((b) >> 3) & 0x7)
#define GET_RM(b)  ((b) & 0x7)
#define ACC_IDX 0
#define FLAGS_IDX 13
#define IP_IDX 12

typedef enum
{
    OP_NONE,
    OP_REGISTER,
    OP_REGISTER_DX,
    OP_REGISTER_CL,
    OP_SREG,
    OP_IMMEDIATE,
    OP_MEMORY,
    OP_MEMORY_DIR, // Direct Address Calculation
    OP_IP_RELATIVE,
    OP_COUNT,
} OperandType;

typedef struct
{
    OperandType type;
    u8 reg_idx;
    u16 immediate_val;
    u8 mem_base_reg;
    s16 mem_disp;
}  Operand;

typedef struct
{
    String8 mnemonic;
    Operand dest;
    Operand src;
    u8 size;
    u8 w_bit;
} Instruction;

Instruction opcode_not_used(t_ctx *ctx);
Instruction modrm_common(t_ctx *ctx);
Instruction modrm_test_xchg_mov(t_ctx *ctx);
Instruction imm_to_rm(t_ctx *ctx);
Instruction imm_to_acc(t_ctx *ctx);
Instruction mov_imm_to_reg(t_ctx *ctx);
Instruction mov_imm_to_mem(t_ctx *ctx);
Instruction mov_mem_to_acc(t_ctx *ctx);
Instruction segment_push_pop(t_ctx *ctx);
Instruction handle_segment_override(t_ctx *ctx);
Instruction handle_daa_das_aaa_aas(t_ctx *ctx);
Instruction handle_inc_dec_push_pop_reg_16(t_ctx *ctx);
Instruction jump(t_ctx *ctx);
Instruction mov_sreg_common(t_ctx *ctx);
Instruction lea_mem_to_reg_16(t_ctx *ctx);
Instruction pop_rm_16(t_ctx *ctx);
Instruction xchg_reg16_to_acc(t_ctx *ctx);
Instruction call_far(t_ctx *ctx);
Instruction handle_cbw_cwd_wait_pushf_popf_sahf_lahf(t_ctx *ctx);
Instruction movs_cmps_stos_lods_scas(t_ctx *ctx);
Instruction test_imm_to_acc(t_ctx *ctx);
Instruction handle_ret(t_ctx *ctx);
Instruction les_lds_mem16_to_reg16(t_ctx *ctx);
Instruction handle_interrupt(t_ctx *ctx);
Instruction rol_ror_rcl_rcr_sal_shr_sar(t_ctx *ctx);
Instruction handle_aam_aad_xlat(t_ctx *ctx);
Instruction esc(t_ctx *ctx);
Instruction loops(t_ctx *ctx);
Instruction in_out_imm8_to_acc(t_ctx *ctx);
Instruction call_jmp_rel(t_ctx *ctx);
Instruction jmp_far(t_ctx *ctx);
Instruction in_out_dx_to_acc(t_ctx *ctx);
Instruction handle_lock_repne_rep_hlt_cmc_clc_stc_cli_sti_cld_std(t_ctx *ctx);
Instruction test_not_neg_mul_imul_div_idiv(t_ctx *ctx);
Instruction handle_inc_dec_rm8(t_ctx *ctx);
Instruction handle_inc_dec_call_jmp_push_16(t_ctx *ctx);

#endif