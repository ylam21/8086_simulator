#ifndef DECODER_H
#define DECODER_H

#include "../common.h"
#include "../utils/io_utils.h"
#include "opcodes.h"

#define GET_MOD(b) (((b) >> 6) & 0x3)
#define GET_REG(b) (((b) >> 3) & 0x7)
#define GET_RM(b)  ((b) & 0x7)

typedef enum
{
    OP_NONE,
    OP_REGISTER,
    OP_IMMEDIATE,
    OP_MEMORY,
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
    u8 size;
    u8 w_bit;

    Operand dest;
    Operand src;
} Instruction;


Instruction opcode_not_used(t_ctx *ctx);

Instruction fmt_modrm_common(t_ctx *ctx);
Instruction fmt_imm_to_acc(t_ctx *ctx);
Instruction fmt_segment_push_pop(t_ctx *ctx);
Instruction handle_segment_override(t_ctx *ctx);
Instruction handle_daa_das_aaa_aas(t_ctx *ctx);
Instruction handle_inc_dec_push_pop_reg_16(t_ctx *ctx);
Instruction fmt_jump(t_ctx *ctx);
Instruction imm_to_rm(t_ctx *ctx);
Instruction fmt_modrm_test_xchg_mov(t_ctx *ctx);
Instruction fmt_mov_sreg_common(t_ctx *ctx);
Instruction fmt_lea_mem_to_reg_16(t_ctx *ctx);
Instruction fmt_pop_rm_16(t_ctx *ctx);
Instruction fmt_xchg_reg16_to_acc(t_ctx *ctx);
Instruction fmt_call_far(t_ctx *ctx);
Instruction handle_cbw_cwd_wait_pushf_popf_sahf_lahf(t_ctx *ctx);
Instruction fmt_mov_mem_to_reg(t_ctx *ctx);
Instruction fmt_movs_cmps_stos_lods_scas(t_ctx *ctx);
Instruction fmt_test_imm_to_acc(t_ctx *ctx);
Instruction mov_imm_to_reg(t_ctx *ctx);
Instruction handle_ret(t_ctx *ctx);
Instruction fmt_les_lds_mem16_to_reg16(t_ctx *ctx);
Instruction fmt_mov_imm_to_mem(t_ctx *ctx);
Instruction handle_interrupt(t_ctx *ctx);
Instruction fmt_rol_ror_rcl_rcr_sal_shr_sar(t_ctx *ctx);
Instruction handle_aam_aad_xlat(t_ctx *ctx);
Instruction fmt_esc(t_ctx *ctx);
Instruction fmt_loops(t_ctx *ctx);
Instruction fmt_in_out_imm8_to_acc(t_ctx *ctx);
Instruction fmt_call_jmp_rel(t_ctx *ctx);
Instruction fmt_jmp_far(t_ctx *ctx);
Instruction fmt_in_out_dx_to_acc(t_ctx *ctx);
Instruction handle_lock_repne_rep_hlt_cmc_clc_stc_cli_sti_cld_std(t_ctx *ctx);
Instruction fmt_test_not_neg_mul_imul_div_idiv(t_ctx *ctx);
Instruction handle_inc_dec_rm8(t_ctx *ctx);
Instruction handle_inc_dec_call_jmp_push_16(t_ctx *ctx);

extern String8 table_reg_w_one[8];
extern String8 table_sreg[4];

#endif