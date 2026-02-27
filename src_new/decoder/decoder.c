#include "decoder.h"
#include "opcodes.h"

static Operand decode_operand_reg(u8 REG)
{
    return (Operand){.type = OP_REGISTER, .reg_idx = REG};
}

static Operand decode_operand_sreg(u8 REG)
{
    return (Operand){.type = OP_SREG, .reg_idx = REG};
}

static Operand decode_operand_mem(u8 W, u8 add_lo, u8 add_hi)
{
    Operand op = {.type = OP_MEMORY_DIR};
    if (W == 0)
    {
        op.mem_disp = add_lo;
    }
    else
    {
        op.mem_disp = add_lo | (add_hi << 8);
    }
    return op;
}

static Operand decode_operand_rm(t_ctx *ctx, u8 MOD, u8 RM)
{
    Operand op = {0};
    if (MOD == 0b00 && RM == 0b110)
    {
        /* DIRECT ADDRESS */
        /* When R/M = 0b110, then 16-bit displacement follows */ 
        op.type = OP_MEMORY_DIR;
        op.mem_disp = ctx->b[2] | (ctx->b[3] << 8);
    }
    else if (MOD == 0b00)
    {
        /* Memory Mode, no displacement */
        op.type = OP_MEMORY;
        op.mem_base_reg = RM;
        op.mem_disp = 0;
    }
    else if (MOD == 0b01)
    {
        /* Memory Mode, 8-bit displacement follows */
        op.type = OP_MEMORY;
        op.mem_base_reg = RM;
        op.mem_disp = (s8)ctx->b[2];
    }
    else if (MOD == 0b10)
    {
        /* Memory Mode, 16-bit displacement follows */
        op.type = OP_MEMORY;
        op.mem_base_reg = RM;
        op.mem_disp = ctx->b[2] | (ctx->b[3] << 8);
    }
    else
    {
        /* Register Mode (no displacement) */
        op = decode_operand_reg(RM);
    }
    return op;
}

// TODO: implement jump table
static u8 match_MODRM_with_offset(u8 MOD, u8 RM)
{
    if (MOD == 0b00)
    {
        /* Memory Mode, no displacement follows
        Except when R/M = 110, then 16-bit displacement follows */ 
        if (RM == 0x6)
        {
            return 4;
        }
        else
        {
            return 2;
        }
    }
    else if (MOD == 0b01)
    {
        /* Memory Mode, 8-bit displacement follows */
        return 3;
    }
    else if (MOD == 0b10)
    {
        /* Memory Mode, 16-bit displacement follows */
        return 4;
    }
    else
    {
        /* Register Mode (no displacement) */
        return 2;
    }
}

Instruction opcode_not_used(t_ctx *ctx)
{
    (void)ctx;
    Instruction inst = {0};
    inst.size = 0;
    return inst;
}


static Operand decode_operand_imm(u8 S, u8 W, u8 *imm_ptr)
{
    Operand op = {0};
    op.type = OP_IMMEDIATE;
    if (W == 0)
    {
        op.immediate_val = *imm_ptr;
    }
    else if (S == 1)
    {
        op.immediate_val = (s16)((s8)imm_ptr[0]);
    }
    else
    {
        op.immediate_val = imm_ptr[0] | (imm_ptr[1] << 8);
    }
    return op;
}

Instruction imm_to_rm(t_ctx *ctx)
{
    String8 mnemonics[8] =
    {
        STR8_LIT("add"),
        STR8_LIT("or"),
        STR8_LIT("adc"),
        STR8_LIT("sbb"),
        STR8_LIT("and"), 
        STR8_LIT("sub"), 
        STR8_LIT("xor"),
        STR8_LIT("cmp"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = (ctx->b[1] >> 3) & 0x7;
    u8 S = (opcode >> 1) & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 W = opcode & 1;
    u8 current_len = match_MODRM_with_offset(MOD, RM);
    u8 *imm_ptr = &ctx->b[current_len];
    u8 imm_len;
    if (W == 1 && S == 0)
    {
        imm_len = 2;
    }
    else
    {
        imm_len = 1;
    }
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.src = decode_operand_imm(S, W, imm_ptr);
    inst.w_bit = W;
    inst.size = current_len + imm_len;
    return inst;
}

Instruction jump(t_ctx *ctx)
{
    String8 mnemonics[16] =
    {
        STR8_LIT("jo") , STR8_LIT( "jno"),
        STR8_LIT("jb") , STR8_LIT( "jnb"),
        STR8_LIT("jz") , STR8_LIT( "jnz"),
        STR8_LIT("jbe"), STR8_LIT("ja"),
        STR8_LIT("js") , STR8_LIT( "jns"),
        STR8_LIT("jp") , STR8_LIT( "jnp"),
        STR8_LIT("jl") , STR8_LIT( "jnl"),
        STR8_LIT("jle"), STR8_LIT( "jg"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0xF;
    s8 IP_INC8 = (s8)ctx->b[1];
    u16 target_address = (u16)(ctx->ip + 2 + IP_INC8);
    Operand dest = {.type = OP_IP_RELATIVE, .immediate_val = target_address};
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.dest = dest;
    inst.size = 2;
    return inst;
}


Instruction xchg_reg16_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 REG = opcode & 0x7;
    Instruction inst = {0};
    if (REG == 0b000)
    {
        inst.mnemonic = STR8_LIT("nop");
    }
    else
    {
        inst.mnemonic = STR8_LIT("xchg");
        inst.w_bit = 1;
        inst.dest = decode_operand_reg(ACC_IDX);
        inst.src = decode_operand_reg(REG);
    }
    inst.size = 1;
    return inst;
}

Instruction handle_inc_dec_push_pop_reg_16(t_ctx *ctx)
{
    String8 mnemonics[4] =
    {
        STR8_LIT("inc"),
        STR8_LIT("dec"),
        STR8_LIT("push"),
        STR8_LIT("pop"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = (opcode >> 3) & 0x3;
    u8 REG = opcode & 0x7;
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.dest = decode_operand_reg(REG);
    inst.w_bit = 1;
    inst.size = 1;
    return inst;
}

Instruction call_far(t_ctx *ctx)
{
    u16 offset = ctx->b[1] | (ctx->b[2] << 8);
    u16 segment = ctx->b[3] | (ctx->b[4] << 8);
    Operand dest = {.type = OP_IMMEDIATE, .immediate_val = segment};
    Operand src = {.type = OP_IMMEDIATE, .immediate_val = offset};
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("call");
    inst.dest = dest;
    inst.src = src;
    inst.size = 5;
    return inst;
}

Instruction handle_cbw_cwd_wait_pushf_popf_sahf_lahf(t_ctx *ctx)
{
    String8 mnemonics[8] = 
    {
        STR8_LIT("cbw"),
        STR8_LIT("cwd"),
        STR8_LIT(""),
        STR8_LIT("wait"),
        STR8_LIT("pushf"), 
        STR8_LIT("popf"), 
        STR8_LIT("sahf"), 
        STR8_LIT("lahf"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0x7;
    Instruction inst = {.mnemonic = mnemonics[idx], .size = 1};
    return inst;
}

Instruction handle_daa_das_aaa_aas(t_ctx *ctx)
{
    String8 mnemonics[4] =
    {
        STR8_LIT("daa"),
        STR8_LIT("das"),
        STR8_LIT("aaa"),
        STR8_LIT("aas"),
    };

    u8 opcode = ctx->b[0];
    u8 idx = (opcode >> 3) & 0x3;
    Instruction inst = {.mnemonic = mnemonics[idx], .size = 1};
    return inst;
}

Instruction handle_segment_override(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    ctx->seg_prefix = (opcode >> 3) & 0x3;
    Instruction inst = {0};
    inst.size = 1;
    return inst;
}

Instruction segment_push_pop(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 1;
    u8 REG = (opcode >> 3) & 0x3;
    Instruction inst = {0};
    if (idx == 0)
    {
        inst.mnemonic = STR8_LIT("push");
    }
    else
    {
        inst.mnemonic = STR8_LIT("pop");
    }
    inst.dest = decode_operand_sreg(REG);
    inst.size = 1;
    return inst;
}

Instruction imm_to_acc(t_ctx *ctx)
{
    String8 mnemonics[8] =
    {
        STR8_LIT("add"),
        STR8_LIT("or"),
        STR8_LIT("adc"),
        STR8_LIT("sbb"),
        STR8_LIT("and"), 
        STR8_LIT("sub"), 
        STR8_LIT("xor"),
        STR8_LIT("cmp"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = (opcode >> 3) & 0x7;
    u8 W = opcode & 1;

    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.dest = decode_operand_reg(ACC_IDX);
    inst.src = decode_operand_imm(0, W, &ctx->b[1]);
    inst.w_bit = W;
    if (W == 0)
    {
        inst.size = 2;
    }
    else
    {
        inst.size = 3;
    }
    return inst;
}

static Instruction modrm(t_ctx *ctx, String8 mnemonic)
{
    u8 opcode = ctx->b[0];
    u8 D = (opcode >> 1) & 1;
    u8 W = opcode & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    
    Instruction inst = {0};
    inst.mnemonic = mnemonic;
    Operand rm = decode_operand_rm(ctx, MOD, RM);
    Operand reg = decode_operand_reg(REG);
    if (D == 0)
    {
        /* Instruction source is specifiend in REG field */   
        inst.src = reg;
        inst.dest = rm;
    }
    else
    {
        /* Instruction destination is specifiend in REG field */   
        inst.dest = reg;
        inst.src = rm;
    }
    inst.w_bit = W;
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}

Instruction pop_rm_16(t_ctx *ctx)
{
    u8 W = 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("pop");
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.size = match_MODRM_with_offset(MOD, RM);
    inst.w_bit = W;
    return inst;
}

Instruction lea_mem_to_reg_16(t_ctx *ctx)
{
    u8 W = 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("lea");
    inst.dest = decode_operand_reg(REG);
    inst.src = decode_operand_rm(ctx, MOD, RM);
    inst.w_bit = W;
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}

Instruction modrm_test_xchg_mov(t_ctx *ctx)
{
    String8 mnemonics[8] =
    {
        STR8_LIT("test"),
        STR8_LIT("test"),
        STR8_LIT("xchg"),
        STR8_LIT("xchg"),
        STR8_LIT("mov"), 
        STR8_LIT("mov"), 
        STR8_LIT("mov"),
        STR8_LIT("mov"),
    };

    u8 opcode = ctx->b[0];
    u8 idx = opcode - 0x84;

    return modrm(ctx, mnemonics[idx]);
}


Instruction modrm_common(t_ctx *ctx)
{
    String8 mnemonics[8] =
    {
        STR8_LIT("add"),
        STR8_LIT("or"),
        STR8_LIT("adc"),
        STR8_LIT("sbb"),
        STR8_LIT("and"), 
        STR8_LIT("sub"), 
        STR8_LIT("xor"),
        STR8_LIT("cmp"),
    };

    u8 opcode = ctx->b[0];
    u8 idx = (opcode >> 3) & 0x7;

    return modrm(ctx, mnemonics[idx]);
}

Instruction mov_sreg_common(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = 1;
    u8 D = (opcode >> 1) & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 SREG = (ctx->b[1] >> 3) & 0x3;
    u8 RM = GET_RM(ctx->b[1]);
    Operand dest;
    Operand src;
    if (D == 0x0)
    {
        /* Instruction source is specifiend in SREG field */   
        dest = decode_operand_rm(ctx, MOD, RM);
        src = decode_operand_sreg(SREG);
    }
    else
    {
        /* Instruction destination is specifiend in SREG field */   
        dest = decode_operand_sreg(SREG);
        src = decode_operand_rm(ctx, MOD, RM);
    }
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("mov");
    inst.dest = dest;
    inst.src = src;
    inst.w_bit = W;
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}

Instruction mov_mem_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 D = (opcode >> 1) & 1; 
    Operand dest;
    Operand src;
    if (D == 0)
    {
        dest = decode_operand_reg(ACC_IDX);
        src = decode_operand_mem(W, ctx->b[1], ctx->b[2]);
    }
    else        
    {
        dest = decode_operand_mem(W, ctx->b[1], ctx->b[2]);
        src = decode_operand_reg(ACC_IDX);
    }
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("mov");
    inst.dest = dest;
    inst.src = src;
    inst.size = 3;
    inst.w_bit = W;
    return inst;
}

Instruction movs_cmps_stos_lods_scas(t_ctx *ctx)
{
    String8 mnemonics_b[6] = 
    {
        STR8_LIT("movsb"), 
        STR8_LIT("cmpsb"), 
        STR8_LIT(""),   
        STR8_LIT("stosb"), 
        STR8_LIT("lodsb"), 
        STR8_LIT("scasb"),  
    }; 
    String8 mnemonics_w[6] = 
    {
        STR8_LIT("movsw"), 
        STR8_LIT("cmpsw"), 
        STR8_LIT(""),   
        STR8_LIT("stosw"), 
        STR8_LIT("lodsw"), 
        STR8_LIT("scasw"),  
    };
    u8 opcode = ctx->b[0];
    u8 idx = (opcode - 0xA4) >> 1;
    u8 W = opcode & 1;
    Instruction inst = {0};
    if (W == 0)
    {
        inst.mnemonic = mnemonics_b[idx];
    }
    else
    {
        inst.mnemonic = mnemonics_w[idx];
    }
    inst.w_bit = W;
    inst.size = 1;
    return inst;
}

Instruction test_imm_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("test");
    inst.dest = decode_operand_reg(ACC_IDX);
    inst.src = decode_operand_imm(0, W, &ctx->b[1]);
    inst.w_bit = W;
    if (W == 0)
    {
        inst.size = 2;
    }
    else
    {
        inst.size = 3;
    }
    return inst;
}

Instruction mov_imm_to_reg(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 REG = opcode & 0x7;
    u8 W = (opcode >> 3) & 1;
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("mov");
    inst.dest = decode_operand_reg(REG);
    inst.src = decode_operand_imm(0, W, &ctx->b[1]);
    inst.w_bit = W;
    if (W == 0)
    {
        inst.size = 2;
    }
    else
    {
        inst.size = 3;
    }
    return inst;
}

Instruction handle_ret(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 is_far = (opcode >> 3) & 1;
    Instruction inst = {0};
    if (is_far == 0)
    {
        inst.mnemonic = STR8_LIT("ret");
    }
    else
    {
        inst.mnemonic = STR8_LIT("retf");
    }
    u8 has_imm = !(opcode & 1); 
    if (has_imm)
    {
        inst.w_bit = 1;
        inst.dest = decode_operand_imm(0, 1, &ctx->b[1]);
        inst.size = 3;
    }
    else
    {
        inst.w_bit = 0;
        inst.size = 1;
    }
    return inst;
}

Instruction les_lds_mem16_to_reg16(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 idx = opcode & 1;
    Instruction inst = {0};
    if (idx == 0)
    {
        inst.mnemonic = STR8_LIT("les");
    }
    else
    {
        inst.mnemonic = STR8_LIT("lds");
    }
    inst.dest = decode_operand_reg(REG);
    inst.src = decode_operand_rm(ctx, MOD, RM);
    inst.size = match_MODRM_with_offset(MOD, RM);
    inst.w_bit = W;
    return inst;
}

Instruction mov_imm_to_mem(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 current_len = match_MODRM_with_offset(MOD, RM);
    u8 *imm_ptr = &ctx->b[current_len];
    u8 imm_len;
    if (W == 1)
    {
        imm_len = 2;
    }
    else
    {
        imm_len = 1;
    }
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("mov");
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.src = decode_operand_imm(0, W, imm_ptr);
    inst.w_bit = W;
    inst.size = current_len + imm_len;
    return inst;
}

Instruction handle_interrupt(t_ctx *ctx)
{
    String8 mnemonics[4] =
    {
        STR8_LIT("int"),
        STR8_LIT("int"),
        STR8_LIT("into"), 
        STR8_LIT("iret"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0x3;
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    Operand dest;
    dest.type = OP_IMMEDIATE;
    if (idx == 0)
    {
        dest.immediate_val = 3;
        inst.size = 1;
    }
    else if (idx == 1)
    {
        dest.immediate_val = ctx->b[1];
        inst.size = 2;
    }
    else 
    {
        inst.size = 1;
    }
    inst.dest = dest;
    return inst;
}

Instruction rol_ror_rcl_rcr_sal_shr_sar(t_ctx *ctx)
{
    String8 mnemonics[8] = 
    {
        STR8_LIT("rol"),
        STR8_LIT("ror"),
        STR8_LIT("rcl"),
        STR8_LIT("rcr"),
        STR8_LIT("sal"), 
        STR8_LIT("shr"), 
        STR8_LIT(""), 
        STR8_LIT("sar"),
    };
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 V = (opcode >> 1) & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    Instruction inst = {0};
    inst.mnemonic = mnemonics[REG];
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.size = match_MODRM_with_offset(MOD, RM);
    inst.w_bit = W;
    Operand src;
    if (V == 0)
    {
        src.type = OP_IMMEDIATE;
        src.immediate_val = 1;
    }
    else
    {
        // Shift/rotate count is specifiend in CL regiset
        src.type = OP_REGISTER_CL;
        src.reg_idx = 1;
    }
    inst.src = src;
    return inst;
}

Instruction handle_aam_aad_xlat(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    Instruction inst = {0};
    if (opcode == 0xD7)
    {
        inst.mnemonic = STR8_LIT("xlat");
        inst.size = 1;
        return inst;
    }
    if (opcode == 0xD4)
    {
        inst.mnemonic = STR8_LIT("aam");
    }
    else
    {
        inst.mnemonic = STR8_LIT("aad");
    }
    inst.size = 2;
    return inst;
}

Instruction esc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 ext_opcode = ((opcode & 0x7) << 3) | REG;
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("esc");
    Operand dest;
    dest.type = OP_IMMEDIATE;
    dest.immediate_val = ext_opcode;
    inst.dest = dest;
    inst.src = decode_operand_rm(ctx, MOD, RM);
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}

Instruction loops(t_ctx *ctx)
{
    String8 mnemonics[4] =
    {
        STR8_LIT("loopnz"),
        STR8_LIT("loopz"),
        STR8_LIT("loop"), 
        STR8_LIT("jcxz"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0x3;
    s8 disp = (s8)ctx->b[1];
    u16 target = (u16)(ctx->ip + 2 + disp);
    Operand dest;
    dest.type = OP_IP_RELATIVE;
    dest.immediate_val = target;
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.dest = dest;
    inst.size = 2;
    return inst;
}

Instruction in_out_dx_to_acc(t_ctx *ctx)
{
    String8 mnemonics[2] =
    {
        STR8_LIT("in"),
        STR8_LIT("out"),
    };
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 is_out = (opcode >> 1) & 1;    
    Operand dest;
    Operand src;
    if (is_out)
    {
        dest.type = OP_REGISTER_DX;
        dest.reg_idx = 2;  // idx for "dx" register table
        src = decode_operand_reg(ACC_IDX);
    }
    else
    {
        dest = decode_operand_reg(ACC_IDX);
        src.type = OP_REGISTER_DX;
        src.reg_idx = 2;
    }
    Instruction inst = {0};
    inst.mnemonic = mnemonics[is_out];
    inst.dest = dest;
    inst.src = src;
    inst.size = 1;
    inst.w_bit = W;
    return inst;
}

Instruction in_out_imm8_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 is_out = (opcode >> 1) & 1; 
    Instruction inst = {0};
    Operand dest;
    Operand src;
    if (is_out)
    {
        inst.mnemonic = STR8_LIT("out");
        dest.type = OP_IMMEDIATE;
        dest.immediate_val = ctx->b[1];
        src = decode_operand_reg(ACC_IDX);
    }
    else
    {
        inst.mnemonic = STR8_LIT("in");
        dest = decode_operand_reg(ACC_IDX);
        src.type = OP_IMMEDIATE;
        src.immediate_val = ctx->b[1];
    }
    inst.dest = dest;
    inst.src = src;
    inst.size = 2;
    inst.w_bit = W;
    return inst;
}

Instruction call_jmp_rel(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("jmp");
    u8 len;
    s32 displacement;
    if (opcode == 0xEB)
    {
        len = 2;
        displacement = (s8)ctx->b[1]; 
    }
    else
    {
        if (opcode == 0xE8)
        {
            inst.mnemonic = STR8_LIT("call");
        } 
        len = 3;
        displacement = (s16)(ctx->b[1] | (ctx->b[2] << 8));
    }
    u16 target = (u16)(ctx->ip + len + displacement);
    Operand dest;
    dest.type = OP_IMMEDIATE;
    dest.immediate_val = target;
    inst.dest = dest;
    inst.size = len;
    return inst;
}

Instruction jmp_far(t_ctx *ctx)
{
    u16 offset = ctx->b[1] | (ctx->b[2] << 8);
    u16 segment = ctx->b[3] | (ctx->b[4] << 8);
    Operand dest = {.type = OP_IMMEDIATE, .immediate_val = segment};
    Operand src = {.type = OP_IMMEDIATE, .immediate_val = offset};
    Instruction inst = {0};
    inst.mnemonic = STR8_LIT("jmp");
    inst.dest = dest;
    inst.src = src;
    inst.size = 5;
    return inst;
}

Instruction handle_lock_repne_rep_hlt_cmc_clc_stc_cli_sti_cld_std(t_ctx *ctx)
{
    String8 mnemonics[14] = 
    {
        STR8_LIT("lock"),
        STR8_LIT(""),
        STR8_LIT("repne"),
        STR8_LIT("rep"),
        STR8_LIT("hlt"),
        STR8_LIT("cmc"),
        STR8_LIT(""),
        STR8_LIT(""),
        STR8_LIT("clc"), 
        STR8_LIT("stc"), 
        STR8_LIT("cli"), 
        STR8_LIT("sti"),
        STR8_LIT("cld"),
        STR8_LIT("std"),
    };
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0xF;
    Instruction inst = {0};
    inst.mnemonic = mnemonics[idx];
    inst.size = 1;
    return inst;
}

Instruction test_not_neg_mul_imul_div_idiv(t_ctx *ctx)
{
    String8 mnemonics[8] = 
    {
        STR8_LIT("test"),
        STR8_LIT(""),
        STR8_LIT("not"),
        STR8_LIT("neg"),
        STR8_LIT("mul"),
        STR8_LIT("imul"),
        STR8_LIT("div"),
        STR8_LIT("idiv"),
    };
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;  
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 current_len = match_MODRM_with_offset(MOD, RM);
    Instruction inst = {0};
    if (REG == 0)
    {
        u8 imm_val;
        u8 imm_len;
        if (W == 0)
        {
            imm_val = ctx->b[current_len];
            imm_len = 1;
        }
        else
        {
            imm_val = ctx->b[current_len] | (ctx->b[current_len + 1] << 8);
            imm_len = 2;
        }
        Operand src = {.type = OP_IMMEDIATE, .immediate_val = imm_val};
        inst.mnemonic = STR8_LIT("test");
        inst.src = src;
        inst.size = current_len + imm_len;
    }
    else
    {
        inst.mnemonic = mnemonics[REG];
        inst.size = current_len;
    }
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.w_bit = W;
    return inst;
}

Instruction handle_inc_dec_rm8(t_ctx *ctx)
{
    String8 mnemonics[2] = 
    {
        STR8_LIT("inc"),
        STR8_LIT("dec"),
    };
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 W = 0; 
    Instruction inst = {0};
    inst.mnemonic = mnemonics[REG];
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.w_bit = W;
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}

Instruction handle_inc_dec_call_jmp_push_16(t_ctx *ctx)
{
    String8 mnemonics[8] = 
    {
        STR8_LIT("inc"),
        STR8_LIT("dec"),
        STR8_LIT("call"),
        STR8_LIT("call"),
        STR8_LIT("jmp"),
        STR8_LIT("jmp"),
        STR8_LIT("push"),
        STR8_LIT(""),
    };
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 W = 1;
    Instruction inst = {0};
    inst.mnemonic = mnemonics[REG];
    inst.dest = decode_operand_rm(ctx, MOD, RM);
    inst.w_bit = W;
    inst.size = match_MODRM_with_offset(MOD, RM);
    return inst;
}