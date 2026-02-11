#include "decoder.h"

u8 *table_reg_w_zero[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
u8 *table_reg_w_one[8]  = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
u8 *table_mem_address_calc[8] = {"bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"};
u8 *table_sreg[4] = {"es", "cs", "ss", "ds"};

static u8 *decode_reg(u8 REG, u8 W)
{
    if (W == 0)
    {
        return (table_reg_w_zero[REG]);
    }
    else
    {
        return (table_reg_w_one[REG]);
    }
}

static u8 *decode_rm(t_ctx *ctx, u8 RM, u8 MOD, u8 W)
{
    u8 *base = table_mem_address_calc[RM];
    u8 *prefix = NULL;

    if (ctx->seg_prefix < 4)
    {
        prefix = table_sreg[ctx->seg_prefix];
    }
   
    if (MOD == 0x0 && RM == 0x6)
    {
        /* DIRECT ADDRESS */
        /* When R/M = 0b110, then 16-bit displacement follows */ 
        u16 disp = ctx->b[2] | (ctx->b[3] << 8);
        return strjoin_fmt(ctx->arena, "%?s[0x%04X]", prefix, disp);
    }
    else if (MOD == 0x0)
    {
        /* Memory Mode, no displacement */
        return strjoin_fmt(ctx->arena, "%?s[%s]", prefix, base);
    }
    else if (MOD == 0x1)
    {
        /* Memory Mode, 8-bit displacement follows */
        s8 disp = (s8)ctx->b[2];
        return strjoin_fmt(ctx->arena, "%?s[%s + %d]", prefix, base, disp);
    }
    else if (MOD == 0x2)
    {
        /* Memory Mode, 16-bit displacement follows */
        u16 disp = ctx->b[2] | (ctx->b[3] << 8);
        return strjoin_fmt(ctx->arena, "%?s[%s + %u]", prefix, base, disp);
    }
    else
    {
        /* Register Mode (no displacement) */
        return decode_reg(RM, W);
    }
}

static u8 match_MODRM_with_offset(u8 MOD, u8 RM)
{
    if (MOD == 0x0)
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
    else if (MOD == 0x1)
    {
        /* Memory Mode, 8-bit displacement follows */
        return 3;
    }
    else if (MOD == 0x2)
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

/* The second byte of a two-byte immediate value is the most significant. */
static u8 *decode_imm_to_reg(t_ctx *ctx, u8 W, u8 data_lo, u8 data_hi)
{
    if (W == 0x0)
    {
        return strjoin_fmt(ctx->arena, "0x%02X", data_lo);
    }
    else
    {
        u16 val = data_lo | (data_hi << 8);
        return strjoin_fmt(ctx->arena, "0x%04X", val);
    }
}

u8 opcode_not_used(t_ctx *ctx)
{
    return 1;
}


static u8 *decode_immediate(t_ctx *ctx, u8 S, u8 W, u8 *imm_ptr)
{
    if (W == 0x0)
    {
        return strjoin_fmt(ctx->arena, "0x%02X", *imm_ptr);
    }
    else if (S == 0x1)
    {
        s16 val = (s16)((s8)imm_ptr[0]);
        return strjoin_fmt(ctx->arena, "0x%04X", (u16)val);
    }
    else
    {
        u16 val = imm_ptr[0] | (imm_ptr[1] << 8);
        return strjoin_fmt(ctx->arena, "0x%04X", val);
    }
}

u8 fmt_imm_to_rm(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"add","or","adc","sbb","and","sub","xor","cmp"};
    u8 idx = (ctx->b[1] >> 3) & 0x7;
    u8 S = (ctx->b[0] >> 1) & 0x1;
    u8 W = ctx->b[0] & 0x1;
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 RM = ctx->b[1] & 0x7;

    u8* field_rm = decode_rm(ctx, RM, MOD, W);

    u8 current_len = match_MODRM_with_offset(MOD, RM);
    u8 *imm_ptr = &ctx->b[current_len];

    u8 *field_imm = decode_immediate(ctx, S, W, imm_ptr); 

    if (MOD != 0x3)    
    {
        if (W == 0x0)
        {
            field_rm = strjoin_fmt(ctx->arena,"%s %s", "byte", field_rm);
        }
        else
        {
            field_rm = strjoin_fmt(ctx->arena,"%s %s", "word", field_rm);
        }
    }

    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_rm, field_imm);
    write_fmt_line(ctx, mnemonics[idx], operands);

    u8 imm_len;
    if (W == 1 && S == 0)
    {
        imm_len = 2;
    }
    else
    {
        imm_len = 1;
    }

    return current_len + imm_len;
}

u8 fmt_jump(t_ctx *ctx)
{
    u8 *mnemonics[16] =
    {
        "jo", "jno",
        "jb", "jnb",
        "je", "jne",
        "jbe","ja",
        "js", "jns",
        "jp", "jnp",
        "jl", "jnl",
        "jle", "jg"
    };

    u8 idx = ctx->b[0] & 0xF;
    s8 IP_INC8 = (s8)ctx->b[1];
    u16 target_address = (u16)(ctx->current_ip + 2 + IP_INC8);

    u8 *target_str = strjoin_fmt(ctx->arena, "0x%04X", target_address);
    write_fmt_line(ctx, mnemonics[idx], target_str);

    return 2;
}


u8 fmt_xchg_reg16_to_acc(t_ctx *ctx)
{
    u8 REG = ctx->b[0] & 0x7;
    
    if (REG == 0x0)
    {
        write_fmt_line_no_operands(ctx, "nop");
    }
    else
    {
        u8 *field_REG = table_reg_w_one[REG];

        u8 *operands = strjoin_fmt(ctx->arena, "ax, %s",field_REG);
        write_fmt_line(ctx, "xchg", operands);
    }
    
    return 1;
}

u8 handle_inc_dec_push_pop_reg_16(t_ctx *ctx)
{
    u8 *mnemonics[4] = {"inc", "dec", "push", "pop"};
    u8 idx = (ctx->b[0] >> 3) & 0x3;
    u8 REG = ctx->b[0] & 0x7;

    write_fmt_line(ctx, mnemonics[idx], table_reg_w_one[REG]);

    return 1;
}

u8 fmt_call_far(t_ctx *ctx)
{
    u16 offset = ctx->b[1] | (ctx->b[2] << 8);
    u16 segment = ctx->b[3] | (ctx->b[4] << 8);

    u8 *operands = strjoin_fmt(ctx->arena, "0x%04X:0x%04X", segment, offset);
    write_fmt_line(ctx, "call", operands);

    return 5;
}

u8 handle_cbw_cwd_wait_pushf_popf_sahf_lahf(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"cbw", "cwd", "","wait", "pushf", "popf", "sahf", "lahf"};
    u8 idx = ctx->b[0] & 0x7;
    
    if (idx == 2)
    {
        return 0;
    }

    write_fmt_line_no_operands(ctx, mnemonics[idx]);

    return 1;
}

u8 handle_daa_das_aaa_aas(t_ctx *ctx)
{
    u8 *mnemonics[4] = {"daa", "das", "aaa", "aas"};
    u8 idx = (ctx->b[0] >> 3) & 0x3;

    write_fmt_line_no_operands(ctx, mnemonics[idx]);
    return 1;
}

u8 handle_segment_override(t_ctx *ctx)
{
    ctx->seg_prefix = (ctx->b[0] >> 3) & 0x3;
    return 1;
}

u8 fmt_segment_push_pop(t_ctx *ctx)
{
    u8 idx = ctx->b[0] & 0x1;
    u8 REG = (ctx->b[0] >> 3) & 0x3;
    u8 *SREG = table_sreg[REG];

    if (idx == 0)
    {
        write_fmt_line(ctx, "push", SREG);
    }
    else
    {
        write_fmt_line(ctx, "pop", SREG);
    }

    return 1;
}

u8 fmt_imm_to_acc(t_ctx *ctx)
{
    u8 *acc_mnemonics[8] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
    u8 idx = (ctx->b[0] >> 3) & 0x7;
    u8 W = ctx->b[0] & 0x1;


    u8 *operands;
    if (W == 0)
    {
        operands = strjoin_fmt(ctx->arena, "%s, 0x%02X", ACC_BYTE, ctx->b[1]);
        write_fmt_line(ctx, acc_mnemonics[idx], operands);
        return 2;
    }
    else
    {
        u16 val = (ctx->b[2] << 8) | ctx->b[1];
        operands = strjoin_fmt(ctx->arena, "%s, 0x%04X", ACC_WORD, val);
        write_fmt_line(ctx, acc_mnemonics[idx], operands);
        return 3;
    }
}

static u8 fmt_modrm(t_ctx *ctx, u8 *mnemonic)
{
    u8 D = (ctx->b[0] >> 1) & 0x1;
    u8 W = ctx->b[0] & 0x1;
    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *field_REG = decode_reg(REG, W);

    u8 *operands;
    if (D == 0)
    {
        /* Instruction source is specifiend in REG field */   
        operands = strjoin_fmt(ctx->arena, "%s, %s", field_RM, field_REG);
    }
    else
    {
        /* Instruction destination is specifiend in REG field */   
        operands = strjoin_fmt(ctx->arena, "%s, %s", field_REG, field_RM);
    }

    write_fmt_line(ctx, mnemonic, operands);

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_pop_rm_16(t_ctx *ctx)
{
    u8 W = 1;
    
    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);

    write_fmt_line(ctx, "pop", field_RM);

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_lea_mem_to_reg_16(t_ctx *ctx)
{
    u8 W = 0x1;

    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *field_REG = decode_reg(REG, W);
    
    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_REG, field_RM);

    write_fmt_line(ctx, "lea", operands);
    
    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_modrm_test_xchg_mov(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"test", "test", "xchg", "xchg", "mov", "mov", "mov", "mov"};
    u8 idx = ctx->b[0] - 0x84;

    return fmt_modrm(ctx, mnemonics[idx]);
}


u8 fmt_modrm_common(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};
    u8 idx = (ctx->b[0] >> 3) & 0x7;

    return fmt_modrm(ctx, mnemonics[idx]);
}

u8 fmt_mov_sreg_common(t_ctx *ctx)
{
    u8 W = 0x1;
    u8 D = (ctx->b[0] >> 1) & 0x1;
    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 RM = ctx->b[1] & 0x7;
    u8 SREG = (ctx->b[1] >> 3) & 0x3;

    u8 *field_rm = decode_rm(ctx, RM, MOD, W);
    u8 *field_sreg = table_sreg[SREG];

    u8 *operands;
    
    if (D == 0x0)
    {
        /* Instruction source is specifiend in SREG field */   
        operands = strjoin_fmt(ctx->arena, "%s, %s", field_rm, field_sreg);
    }
    else
    {
        /* Instruction destination is specifiend in SREG field */   
        operands = strjoin_fmt(ctx->arena, "%s, %s", field_sreg, field_rm);
    }

    write_fmt_line(ctx, "mov", operands);
    
    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_mov_mem_to_reg(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 0x1;
    u8 D = (opcode >> 1) & 0x1; 

    u16 addr = ctx->b[1] | (ctx->b[2] << 8);
    
    u8 *prefix_str;
    
    if (ctx->seg_prefix < 4)
    {
        prefix_str = table_sreg[ctx->seg_prefix];
    }
    else
    {
        prefix_str = (u8 *)"ds";
    }

    u8 *mem_op = strjoin_fmt(ctx->arena, "%?s[0x%04X]", prefix_str, addr);
    
    u8 *reg_op;
    
    if (W == 0x0)
    {
        reg_op = ACC_BYTE;
    }
    else
    {
        reg_op = ACC_WORD;
    }
    
    u8 *operands;
    if (D == 0)
        operands = strjoin_fmt(ctx->arena, "%s, %s", reg_op, mem_op);
    else        
        operands = strjoin_fmt(ctx->arena, "%s, %s", mem_op, reg_op);

    write_fmt_line(ctx, "mov", operands);

    return 3; 
}

u8 fmt_movs_cmps_stos_lods_scas(t_ctx *ctx)
{
    char *base_mnemonics[6] = 
    {
        "movs", 
        "cmps", 
        NULL,   
        "stos", 
        "lods", 
        "scas"  
    };

    u8 opcode = ctx->b[0];
    u8 idx = (opcode - 0xA4) >> 1;

    if (idx == 2)
    {
        return 0; 
    }

    u8 W = opcode & 0x1;
    u8 suffix;
    if (W == 0x0)
    {
        suffix = 'b';
    }
    else
    {
        suffix = 'w';
    }
    
    u8 *full_mnemonic = strjoin_fmt(ctx->arena, "%s%c", base_mnemonics[idx], suffix);
    
    write_fmt_line_no_operands(ctx, full_mnemonic);

    return 1;
}

u8 fmt_test_imm_to_acc(t_ctx *ctx)
{
    u8 W = ctx->b[0] & 0x1;

    u8 *operands;
    if (W == 0)
    {
        operands = strjoin_fmt(ctx->arena, "%s, 0x%02X", ACC_BYTE, ctx->b[1]);
        write_fmt_line(ctx, "test", operands);
        return 2;
    }
    else
    {
        u16 val = (ctx->b[2] << 8) | ctx->b[1];
        operands = strjoin_fmt(ctx->arena, "%s, 0x%04X", ACC_WORD, val);
        write_fmt_line(ctx, "test", operands);
        return 3;
    }
}

u8 fmt_mov_imm_to_reg(t_ctx *ctx)
{
    u8 REG = ctx->b[0] & 0x7;
    u8 W = (ctx->b[0] >> 3) & 0x1;

    u8 *field_IMM = decode_imm_to_reg(ctx, W, ctx->b[1], ctx->b[2]);
    u8 *field_REG = decode_reg(REG, W);

    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_REG, field_IMM);

    write_fmt_line(ctx, "mov", operands);

    if (W == 0x0)
    {
        return 2;
    }
    else
    {
        return 3;
    }
}

u8 handle_ret(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    
    u8 is_far = (opcode >> 3) & 0x1;
    
    u8 *mnemonic;
    if (is_far == 0x0)
    {
        mnemonic = "ret";
    }
    else
    {
        mnemonic = "retf";
    }

    u8 has_imm = !(opcode & 0x1); 

    if (has_imm)
    {
        u16 val = ctx->b[1] | (ctx->b[2] << 8);
        u8 *imm_str = strjoin_fmt(ctx->arena, "0x%04X", val);
        write_fmt_line(ctx, mnemonic, imm_str);
        return 3;
    }
    else
    {
        write_fmt_line_no_operands(ctx, mnemonic);
        return 1;
    }
}

u8 fmt_les_lds_mem16_to_reg16(t_ctx *ctx)
{
    u8 W = 0x1;

    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *field_REG = decode_reg(REG, W);

    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_REG, field_RM);
    u8 idx = ctx->b[0] & 0x1;
    if (idx == 0x0)
    {
        write_fmt_line(ctx, "les", operands);
    }
    else
    {
        write_fmt_line(ctx, "lds", operands);
    }

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_mov_imm_to_mem(t_ctx *ctx)
{
    u8 W = ctx->b[0] & 0x1;
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 RM = ctx->b[1] & 0x7;

    u8* field_rm = decode_rm(ctx, RM, MOD, W);

    u8 current_len = match_MODRM_with_offset(MOD, RM);
    u8 *imm_ptr = &ctx->b[current_len];

    u8 *field_imm = decode_immediate(ctx, 0, W, imm_ptr); 

    if (MOD != 0x3)    
    {
        if (W == 0x0)
        {
            field_rm = strjoin_fmt(ctx->arena,"%s %s", "byte", field_rm);
        }
        else
        {
            field_rm = strjoin_fmt(ctx->arena,"%s %s", "word", field_rm);
        }
    }

    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_rm, field_imm);
    write_fmt_line(ctx, "mov", operands);

    u8 imm_len;
    if (W == 1)
    {
        imm_len = 2;
    }
    else
    {
        imm_len = 1;
    }

    return current_len + imm_len;
}

u8 handle_interrupt(t_ctx *ctx)
{
    u8 *mnemonics[4] = {"int", "int", "into", "iret"};
    u8 idx = ctx->b[0] & 0x3;

    u8 *field_IMM;
    if (idx == 0x0)
    {
        field_IMM = strjoin_fmt(ctx->arena, "%u", 3);
        write_fmt_line(ctx, mnemonics[idx], field_IMM);
        return 1;
    }
    else if (idx == 0x1)
    {
        field_IMM = strjoin_fmt(ctx->arena, "0x%02X", ctx->b[1]);
        write_fmt_line(ctx,mnemonics[idx], field_IMM);
        return 2;
    }
    else 
    {
        write_fmt_line_no_operands(ctx, mnemonics[idx]);
        return 1;
    }
}

u8 fmt_rol_ror_rcl_rcr_sal_shr_sar(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"rol", "ror", "rcl", "rcr", "sal", "shr","", "sar"};

    u8 W = ctx->b[0] & 0x1;
    u8 V = (ctx->b[0] >> 1) & 0x1;
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);

    u8 *operands;
    if (V == 0x0)
    {
        operands = strjoin_fmt(ctx->arena, "%s, %u", field_RM, 1u);
    }
    else
    {
        operands = strjoin_fmt(ctx->arena, "%s, cl", field_RM);
    }

    write_fmt_line(ctx, mnemonics[REG], operands);

    return match_MODRM_with_offset(MOD, RM);
}

u8 handle_aam_aad_xlat(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];

    if (opcode == 0xD7)
    {
        write_fmt_line_no_operands(ctx, "xlat");
        return 1;
    }

    u8 *mnemonic;
    if (opcode == 0xD4)
    {
        mnemonic = "aam";
    }
    else
    {
        mnemonic = "aad";
    }

    u8 imm = ctx->b[1];
    u8 *operands = strjoin_fmt(ctx->arena, "0x%02X", imm);
    
    write_fmt_line(ctx, mnemonic, operands);

    return 2;
}

u8 fmt_esc(t_ctx *ctx)
{
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 RM  = ctx->b[1] & 0x7;
    u8 REG = (ctx->b[1] >> 3) & 0x7;

    u8 ext_opcode = ((ctx->b[0] & 0x7) << 3) | REG;

    u8 *rm_field = decode_rm(ctx, RM, MOD, 1);

    u8 *operands = strjoin_fmt(ctx->arena, "0x%02X, %s", ext_opcode, rm_field);
    
    write_fmt_line(ctx, "esc", operands);

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_loops(t_ctx *ctx)
{
    u8 *mnemonics[4] = {"loopne", "loope", "loop", "jcxz"};
    u8 idx = ctx->b[0] & 0x3;

    s8 disp = (s8)ctx->b[1];
    u16 target = (u16)(ctx->current_ip + 2 + disp);
    u8 *operands = strjoin_fmt(ctx->arena, "0x%04X", target);
    
    write_fmt_line(ctx, mnemonics[idx], operands);

    return 2;
}

u8 fmt_in_out_dx_to_acc(t_ctx *ctx)
{
    u8 *mnemonics[2] = {"in", "out"};
    u8 is_out = (ctx->b[0] >> 1) & 0x1;    

    u8 W = ctx->b[0] & 0x1;
    u8 *field_REG;
    if (W == 0x0)
    {
        field_REG = "al";
    }
    else
    {
        field_REG = "ax";
    }

    u8 *operands;
    if (is_out)
    {
        operands = strjoin_fmt(ctx->arena, "dx, %s", field_REG);
    }
    else
    {
        operands = strjoin_fmt(ctx->arena, "%s, dx", field_REG);
    }

    write_fmt_line(ctx, mnemonics[is_out], operands);

    return 1;
}

u8 fmt_in_out_imm8_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 0x1;
    u8 is_out = (opcode >> 1) & 0x1; 

    u8 *acc_str;
    if (W == 0x0)    
    {
        acc_str = "al";
    }
    else
    {
        acc_str = "ax";
    }

    u8 *port_str = strjoin_fmt(ctx->arena, "0x%02X", ctx->b[1]);
    
    u8 *operands;
    if (is_out)
    {
        operands = strjoin_fmt(ctx->arena, "%s, %s", port_str, acc_str);
        write_fmt_line(ctx, "out", operands);
    }
    else
    {
        operands = strjoin_fmt(ctx->arena, "%s, %s", acc_str, port_str);
        write_fmt_line(ctx, "in", operands);
    }

    return 2;
}

u8 fmt_call_jmp_rel(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    
    u8 *mnemonic = "jmp";
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
            mnemonic = "call";
        } 
        len = 3;
        displacement = (s16)(ctx->b[1] | (ctx->b[2] << 8));
    }

    u16 target = (u16)(ctx->current_ip + len + displacement);

    u8 *operands = strjoin_fmt(ctx->arena, "0x%04X", target);
    write_fmt_line(ctx, mnemonic, operands);

    return len;
}

u8 fmt_jmp_far(t_ctx *ctx)
{
    u16 offset = ctx->b[1] | (ctx->b[2] << 8);
    u16 segment = ctx->b[3] | (ctx->b[4] << 8);

    u8 *operands = strjoin_fmt(ctx->arena, "0x%04X0x%04X", segment, offset);
    write_fmt_line(ctx, "jmp", operands);

    return 5; 
}

u8 handle_lock_repne_rep_hlt_cmc_clc_stc_cli_sti_cld_std(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    
    if (opcode == 0xF6 || opcode == 0xF7 || opcode == 0xFE || opcode == 0xFF)
    {
        return 0; 
    }

    u8 *mnemonics[14] = {
        "lock", "", "repne", "rep",
        "hlt", "cmc", "", "",  
        "clc", "stc", "cli", "sti",
        "cld", "std"
    };

    u8 idx = opcode & 0xF;

    if (idx == 1 || mnemonics[idx][0] == '\0')
    {
        return 0;
    }

    write_fmt_line_no_operands(ctx, mnemonics[idx]);

    return 1;
}

u8 fmt_test_not_neg_mul_imul_div_idiv(t_ctx *ctx)
{
    u8 *mnemonics[8] = {"test", "", "not", "neg", "mul", "imul", "div", "idiv"};

    u8 W = ctx->b[0] & 0x1;  
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 REG = (ctx->b[1] >> 3) & 0x7; 
    u8 RM  = ctx->b[1] & 0x7;

    if (REG == 1)
    {
        return 0;
    }

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 current_len = match_MODRM_with_offset(MOD, RM);

    if (REG == 0)
    {
        u8 *imm_val;
        u8 imm_len;

        if (W == 0)
        {
            imm_val = strjoin_fmt(ctx->arena, "0x%02X", ctx->b[current_len]);
            imm_len = 1;
        }
        else
        {
            u16 val = ctx->b[current_len] | (ctx->b[current_len + 1] << 8);
            imm_val = strjoin_fmt(ctx->arena, "0x%04X", val);
            imm_len = 2;
        }

        u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_RM, imm_val);
        write_fmt_line(ctx, "test", operands);
        
        return current_len + imm_len;
    }
    
    else
    {
        write_fmt_line(ctx, mnemonics[REG], field_RM);
        return current_len;
    }
}

u8 handle_inc_dec_rm8(t_ctx *ctx)
{
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM  = ctx->b[1] & 0x7;

    if (REG > 1) return 0;

    u8 *mnemonics[2] = {"inc", "dec"};
    
    u8 W = 0; 

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    
    if (MOD != 3) 
    {
        field_RM = strjoin_fmt(ctx->arena, "byte %s", field_RM);
    }

    write_fmt_line(ctx, mnemonics[REG], field_RM);

    return match_MODRM_with_offset(MOD, RM);
}

u8 handle_inc_dec_call_jmp_push_16(t_ctx *ctx)
{
    u8 MOD = (ctx->b[1] >> 6) & 0x3;
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM  = ctx->b[1] & 0x7;

    if (REG == 0x7) return 0;

    u8 *mnemonics[8] = {
        "inc", "dec", "call", "call", "jmp", "jmp", "push", ""
    };

    u8 W = 0x1;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *final_operand = field_RM;

    if (REG == 0x3 || REG == 0x5)
    {
        final_operand = strjoin_fmt(ctx->arena, "far %s", field_RM);
    }
    else if (MOD != 3)
    {
        if (REG <= 0x1 || REG == 0x6) 
        {
            final_operand = strjoin_fmt(ctx->arena, "word %s", field_RM);
        }
    }

    write_fmt_line(ctx, mnemonics[REG], final_operand);

    return match_MODRM_with_offset(MOD, RM);
}