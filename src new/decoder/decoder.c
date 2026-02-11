#include "decoder.h"

// TODO: RENAME FILE TO SIMULATOR
// TODO: WHERE TO MOVE THESE TABLES

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

static String8 decode_reg(u8 REG, u8 W)
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

static String8 decode_rm(t_ctx *ctx, u8 RM, u8 MOD, u8 W)
{
    String8 base = table_mem_address_calc[RM];
    String8 prefix = (String8){0};

    if (ctx->seg_prefix < 4)
    {
        prefix = table_sreg[ctx->seg_prefix];
    }
   
    if (MOD == 0b00 && RM == 0b110)
    {
        /* DIRECT ADDRESS */
        /* When R/M = 0b110, then 16-bit displacement follows */ 
        u16 disp = ctx->b[2] | (ctx->b[3] << 8);
        return str8_fmt(ctx->arena, STR8_LIT("%?s[0x%04X]"), prefix, disp);
    }
    else if (MOD == 0b00)
    {
        /* Memory Mode, no displacement */
        return str8_fmt(ctx->arena, STR8_LIT("%?s[%s]"), prefix, base);
    }
    else if (MOD == 0b01)
    {
        /* Memory Mode, 8-bit displacement follows */
        s8 disp = (s8)ctx->b[2];
        return str8_fmt(ctx->arena, STR8_LIT("%?s[%s + %d]"), prefix, base, disp);
    }
    else if (MOD == 0b10)
    {
        /* Memory Mode, 16-bit displacement follows */
        u16 disp = ctx->b[2] | (ctx->b[3] << 8);
        return str8_fmt(ctx->arena, STR8_LIT("%?s[%s + %u]"), prefix, base, disp);
    }
    else
    {
        /* Register Mode (no displacement) */
        return decode_reg(RM, W);
    }
}

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

/* The second byte of a two-byte immediate value is the most significant. */
static String8 decode_imm_to_reg(t_ctx *ctx, u8 W, u8 data_lo, u8 data_hi)
{
    if (W == 0x0)
    {
        return str8_fmt(ctx->arena, STR8_LIT("0x%02X"), data_lo);
    }
    else
    {
        u16 val = data_lo | (data_hi << 8);
        return str8_fmt(ctx->arena, STR8_LIT("0x%04X"), val);
    }
}

u8 opcode_not_used(t_ctx *ctx)
{
    return 1;
}


static String8 decode_immediate(t_ctx *ctx, u8 S, u8 W, u8 *imm_ptr)
{
    if (W == 0)
    {
        return str8_fmt(ctx->arena, STR8_LIT("0x%02X"), *imm_ptr);
    }
    else if (S == 1)
    {
        s16 val = (s16)((s8)imm_ptr[0]);
        return str8_fmt(ctx->arena, STR8_LIT("0x%04X"), (u16)val);
    }
    else
    {
        u16 val = imm_ptr[0] | (imm_ptr[1] << 8);
        return str8_fmt(ctx->arena, STR8_LIT("0x%04X"), val);
    }
}

u8 fmt_imm_to_rm(t_ctx *ctx)
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
    u8 W = opcode & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);

    String8 field_rm = decode_rm(ctx, RM, MOD, W);

    u8 current_len = match_MODRM_with_offset(MOD, RM);
    u8 *imm_ptr = &ctx->b[current_len];

    String8 field_imm = decode_immediate(ctx, S, W, imm_ptr); 

    if (MOD != 0b11)    
    {
        if (W == 0)
        {
            field_rm = str8_fmt(ctx->arena, STR8_LIT("%s %s"), STR8_LIT("byte"), field_rm);
        }
        else
        {
            field_rm = str8_fmt(ctx->arena, STR8_LIT("%s %s"), STR8_LIT("word"), field_rm);
        }
    }

    String8 operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_rm, field_imm);
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
    String8 mnemonics[16] =
    {
        STR8_LIT("jo") , STR8_LIT( "jno"),
        STR8_LIT("jb") , STR8_LIT( "jnb"),
        STR8_LIT("je") , STR8_LIT( "jne"),
        STR8_LIT("jbe"), STR8_LIT("ja"),
        STR8_LIT("js") , STR8_LIT( "jns"),
        STR8_LIT("jp") , STR8_LIT( "jnp"),
        STR8_LIT("jl") , STR8_LIT( "jnl"),
        STR8_LIT("jle"), STR8_LIT( "jg"),
    };

    u8 opcode = ctx->b[0];
    u8 idx = opcode & 0xF;
    s8 IP_INC8 = (s8)ctx->b[1];
    u16 target_address = (u16)(ctx->current_ip + 2 + IP_INC8);

    String8 target_str = str8_fmt(ctx->arena, STR8_LIT("0x%04X"), target_address);
    write_fmt_line(ctx, mnemonics[idx], target_str);

    return 2;
}


u8 fmt_xchg_reg16_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 REG = opcode & 0x7;
    
    if (REG == 0b000)
    {
        write_fmt_line_no_operands(ctx, STR8_LIT("nop"));
    }
    else
    {
        String8 field_REG = table_reg_w_one[REG];
        String8 operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), ACC_WORD, field_REG);
        write_fmt_line(ctx, STR8_LIT("xchg"), operands);
    }
    
    return 1;
}

u8 handle_inc_dec_push_pop_reg_16(t_ctx *ctx)
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

    write_fmt_line(ctx, mnemonics[idx], table_reg_w_one[REG]);

    return 1;
}

u8 fmt_call_far(t_ctx *ctx)
{
    u16 offset = ctx->b[1] | (ctx->b[2] << 8);
    u16 segment = ctx->b[3] | (ctx->b[4] << 8);

    String8 operands = str8_fmt(ctx->arena, STR8_LIT("0x%04X:0x%04X"), segment, offset);
    write_fmt_line(ctx, STR8_LIT("call"), operands);

    return 5;
}

u8 handle_cbw_cwd_wait_pushf_popf_sahf_lahf(t_ctx *ctx)
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
    
    if (idx == 0b010)
    {
        return 0;
    }

    write_fmt_line_no_operands(ctx, mnemonics[idx]);

    return 1;
}

u8 handle_daa_das_aaa_aas(t_ctx *ctx)
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

    write_fmt_line_no_operands(ctx, mnemonics[idx]);
    return 1;
}

u8 handle_segment_override(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    ctx->seg_prefix = (opcode >> 3) & 0x3;
    return 1;
}

u8 fmt_segment_push_pop(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 idx = opcode & 1;
    u8 REG = (opcode >> 3) & 0x3;
    String8 SREG = table_sreg[REG];

    if (idx == 0)
    {
        write_fmt_line(ctx, STR8_LIT("push"), SREG);
    }
    else
    {
        write_fmt_line(ctx, STR8_LIT("pop"), SREG);
    }

    return 1;
}

u8 fmt_imm_to_acc(t_ctx *ctx)
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

    String8 operands;
    if (W == 0)
    {
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, 0x%02X"), ACC_BYTE, ctx->b[1]);
        write_fmt_line(ctx, mnemonics[idx], operands);
        return 2;
    }
    else
    {
        u16 val = (ctx->b[2] << 8) | ctx->b[1];
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, 0x%04X"), ACC_WORD, val);
        write_fmt_line(ctx, mnemonics[idx], operands);
        return 3;
    }
}

static u8 fmt_modrm(t_ctx *ctx, String8 mnemonic)
{
    u8 opcode = ctx->b[0];
    u8 D = (opcode >> 1) & 1;
    u8 W = opcode & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);

    String8 field_RM = decode_rm(ctx, RM, MOD, W);
    String8 field_REG = decode_reg(REG, W);

    String8 operands;
    if (D == 0)
    {
        /* Instruction source is specifiend in REG field */   
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_RM, field_REG);
    }
    else
    {
        /* Instruction destination is specifiend in REG field */   
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_REG, field_RM);
    }

    write_fmt_line(ctx, mnemonic, operands);

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_pop_rm_16(t_ctx *ctx)
{
    u8 W = 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);

    String8 field_RM = decode_rm(ctx, RM, MOD, W);

    write_fmt_line(ctx, STR8_LIT("pop"), field_RM);

    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_lea_mem_to_reg_16(t_ctx *ctx)
{
    u8 W = 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 REG = GET_REG(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);

    String8 field_RM = decode_rm(ctx, RM, MOD, W);
    String8 field_REG = decode_reg(REG, W);
   
    String8 operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_REG, field_RM);

    write_fmt_line(ctx, STR8_LIT("lea"), operands);
    
    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_modrm_test_xchg_mov(t_ctx *ctx)
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

    return fmt_modrm(ctx, mnemonics[idx]);
}


u8 fmt_modrm_common(t_ctx *ctx)
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

    return fmt_modrm(ctx, mnemonics[idx]);
}

u8 fmt_mov_sreg_common(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = 1;
    u8 D = (opcode >> 1) & 1;
    u8 MOD = GET_MOD(ctx->b[1]);
    u8 RM = GET_RM(ctx->b[1]);
    u8 SREG = (ctx->b[1] >> 3) & 0x3;

    String8 field_RM = decode_rm(ctx, RM, MOD, W);
    String8 field_SREG = table_sreg[SREG];

    String8 operands;
    if (D == 0x0)
    {
        /* Instruction source is specifiend in SREG field */   
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_RM, field_SREG);
    }
    else
    {
        /* Instruction destination is specifiend in SREG field */   
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_SREG, field_RM);
    }

    write_fmt_line(ctx, STR8_LIT("mov"), operands);
    
    return match_MODRM_with_offset(MOD, RM);
}

u8 fmt_mov_mem_to_reg(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;
    u8 D = (opcode >> 1) & 1; 

    u16 addr = ctx->b[1] | (ctx->b[2] << 8);
    
    String8 prefix_str;
    if (ctx->seg_prefix < 4)
    {
        prefix_str = table_sreg[ctx->seg_prefix];
    }
    else
    {
        prefix_str = STR8_LIT("ds");
    }

    String8 mem_op = str8_fmt(ctx->arena, STR8_LIT("%?s[0x%04X]"), prefix_str, addr);
    
    String8 reg_op;
    if (W == 0x0)
    {
        reg_op = ACC_BYTE;
    }
    else
    {
        reg_op = ACC_WORD;
    }
    
    String8 operands;
    if (D == 0)
    {
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), reg_op, mem_op);
    }
    else        
    {
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), mem_op, reg_op);
    }

    write_fmt_line(ctx, STR8_LIT("mov"), operands);

    return 3; 
}

u8 fmt_movs_cmps_stos_lods_scas(t_ctx *ctx)
{
    String8 base_mnemonics[6] = 
    {
        STR8_LIT("movs"), 
        STR8_LIT("cmps"), 
        STR8_LIT(""),   
        STR8_LIT("stos"), 
        STR8_LIT("lods"), 
        STR8_LIT("scas"),  
    };

    u8 opcode = ctx->b[0];
    u8 idx = (opcode - 0xA4) >> 1;

    if (idx == 2)
    {
        return 0; 
    }

    u8 W = opcode & 1;
    u8 suffix;
    if (W == 0x0)
    {
        suffix = 'b';
    }
    else
    {
        suffix = 'w';
    }
    
    String8 full_mnemonic = str8_fmt(ctx->arena, STR8_LIT("%s%c"), base_mnemonics[idx], suffix);
    
    write_fmt_line_no_operands(ctx, full_mnemonic);

    return 1;
}

u8 fmt_test_imm_to_acc(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 W = opcode & 1;

    String8 operands;
    if (W == 0)
    {
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, 0x%02X"), ACC_BYTE, ctx->b[1]);
        write_fmt_line(ctx, STR8_LIT("test"), operands);
        return 2;
    }
    else
    {
        u16 val = (ctx->b[2] << 8) | ctx->b[1];
        operands = str8_fmt(ctx->arena, STR8_LIT("%s, 0x%04X"), ACC_WORD, val);
        write_fmt_line(ctx, STR8_LIT("test"), operands);
        return 3;
    }
}

u8 fmt_mov_imm_to_reg(t_ctx *ctx)
{
    u8 opcode = ctx->b[0];
    u8 REG = opcode & 0x7;
    u8 W = (opcode >> 3) & 1;

    String8 field_IMM = decode_imm_to_reg(ctx, W, ctx->b[1], ctx->b[2]);
    String8 field_REG = decode_reg(REG, W);

    String8 operands = str8_fmt(ctx->arena, STR8_LIT("%s, %s"), field_REG, field_IMM);

    write_fmt_line(ctx, STR8_LIT("mov"), operands);

    if (W == 0)
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
    
    u8 is_far = (opcode >> 3) & 1;
    
    u8 *mnemonic;
    if (is_far == 0x0)
    {
        mnemonic = "ret";
    }
    else
    {
        mnemonic = "retf";
    }

    u8 has_imm = !(opcode & 1); 

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
    u8 W = 1;

    u8 MOD = (ctx->b[1] >> 6) & 0x3; 
    u8 REG = (ctx->b[1] >> 3) & 0x7;
    u8 RM = ctx->b[1] & 0x7;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *field_REG = decode_reg(REG, W);

    u8 *operands = strjoin_fmt(ctx->arena, "%s, %s", field_REG, field_RM);
    u8 idx = ctx->b[0] & 1;
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
    u8 opcode = ctx->b[0];
    u8 W = ctx->b[0] & 1;
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
    u8 opcode = ctx->b[0];
    u8 *mnemonics[4] = {"int", "int", "into", "iret"};
    u8 idx = ctx->b[0] & 0x3;

    u8 *field_IMM;
    if (idx == 0x0)
    {
        field_IMM = strjoin_fmt(ctx->arena, "%u", 3);
        write_fmt_line(ctx, mnemonics[idx], field_IMM);
        return 1;
    }
    else if (idx == 1)
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

    u8 opcode = ctx->b[0];
    u8 W = ctx->b[0] & 1;
    u8 V = (ctx->b[0] >> 1) & 1;
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
    u8 opcode = ctx->b[0];
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
    u8 is_out = (ctx->b[0] >> 1) & 1;    

    u8 opcode = ctx->b[0];
    u8 W = ctx->b[0] & 1;
    String8 field_REG;
    if (W == 0x0)
    {
        field_REG = ACC_BYTE;
    }
    else
    {
        field_REG = ACC_WORD;
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
    u8 W = opcode & 1;
    u8 is_out = (opcode >> 1) & 1; 

    String8 acc_str;
    if (W == 0x0)    
    {
        acc_str = ACC_BYTE;
    }
    else
    {
        acc_str = ACC_WORD;
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

    u8 opcode = ctx->b[0];
    u8 W = ctx->b[0] & 1;  
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

    u8 W = 1;

    u8 *field_RM = decode_rm(ctx, RM, MOD, W);
    u8 *final_operand = field_RM;

    if (REG == 0x3 || REG == 0x5)
    {
        final_operand = strjoin_fmt(ctx->arena, "far %s", field_RM);
    }
    else if (MOD != 3)
    {
        if (REG <= 1 || REG == 0x6) 
        {
            final_operand = strjoin_fmt(ctx->arena, "word %s", field_RM);
        }
    }

    write_fmt_line(ctx, mnemonics[REG], final_operand);

    return match_MODRM_with_offset(MOD, RM);
}