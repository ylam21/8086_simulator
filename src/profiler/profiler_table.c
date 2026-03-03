u8 isTypeReg(OperandType t)
{
	return (t == OP_REGISTER || t == OP_REGISTER_CL || t == OP_REGISTER_DX);
}

u8 prof_mov_imm_to_reg(prof_ctx *ctx)
{
	(void)ctx;
	return 4;
}

u8 prof_opcode_not_used(prof_ctx *ctx)
{
	(void)ctx;
	return 0;
}

u8 prof_modrm_common(prof_ctx *ctx)
{
	u8 offsets[3] =
	{
		[OFFSET_REG_TO_REG] = 3,
		[OFFSET_MEM_TO_REG] = 9,
		[OFFSET_REG_TO_MEM] = 16,
	};
	u8 penalty[3] =
	{
		[OFFSET_REG_TO_REG] = 0,
		[OFFSET_MEM_TO_REG] = 1,
		[OFFSET_REG_TO_MEM] = 2,
	};
	return prof_modrm(ctx, offsets, penalty);
}

u8 prof_modrm_test_xchg_mov(prof_ctx *ctx)
{
	u8 offsets[3] = 
	{
		[OFFSET_REG_TO_REG] = 2,
		[OFFSET_MEM_TO_REG] = 8,
		[OFFSET_REG_TO_MEM] = 9,
	};
	u8 penalty[3] =
	{
		[OFFSET_REG_TO_REG] = 0,
		[OFFSET_MEM_TO_REG] = 1,
		[OFFSET_REG_TO_MEM] = 2,
	};
	return prof_modrm(ctx, offsets, penalty);
}

// NOTE: offsets is always contains 3 u8's
u8 prof_modrm(prof_ctx *ctx, u8 *offsets, u8 *transfer_penalty)
{
	OperandType dType = ctx->inst.dest.type;
	OperandType sType = ctx->inst.src.type;
	u8 ea_field = 0;
	u8 offset = 0;
	
	if (isTypeReg(dType) && isTypeReg(sType))
	{
		return offsets[OFFSET_REG_TO_REG];
	}
	else if (isTypeReg(dType) && (sType == OP_MEMORY || sType == OP_MEMORY_DIR))
	{
		ea_field = calc_EA_field(&ctx->inst.src);
		offset = offsets[OFFSET_MEM_TO_REG];
		if (ctx->inst.src.mem_disp & 1) // the address is odd
		{
			u8 penalty = transfer_penalty[OFFSET_MEM_TO_REG] << 2;
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea + %up)"), offset, ea_field,penalty);
			return offset + ea_field + penalty;
		}
		else
		{
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea)"), offset, ea_field);
			return  offset + ea_field;
		}
	}
	else if ((dType == OP_MEMORY || dType == OP_MEMORY_DIR) && isTypeReg(sType))
	{
		ea_field = calc_EA_field(&ctx->inst.dest);
		offset = offsets[OFFSET_REG_TO_MEM];
		if (ctx->inst.dest.mem_disp & 1) // the address is odd
		{
			u8 penalty = transfer_penalty[OFFSET_REG_TO_MEM] << 2;
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea + %up)"), offset, ea_field,penalty);
			return offset + ea_field + penalty;
		}
		else
		{
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea)"), offset, ea_field);
			return  offset + ea_field;
		}
	}
	return 0;
}


u8 calc_EA_field(Operand *op)
{
	if (op->type == OP_MEMORY_DIR)
	{
		return 6;
	}
	else
	{
		if (op->mem_disp)
		{
			return match_EA_component_with_clocks[op->mem_base_reg] + 4;
		}
		else
		{
			return match_EA_component_with_clocks[op->mem_base_reg];
		}
	}
}

u8 match_EA_component_with_clocks[RM_EAC_CNT] =
{
	[RM_BX_SI] = 7,
	[RM_BX_DI] = 8,
	[RM_BP_SI] = 8,
	[RM_BP_DI] = 7,
	[RM_SI] = 5,
	[RM_DI] = 5,
	[RM_BP] = 5,
	[RM_BX] = 5,
};

u8 prof_imm_to_rm(prof_ctx *ctx)
{
	OperandType dType = ctx->inst.dest.type;

	if (isTypeReg(dType))
	{
		return 4;
	}
	else
	{		
		u8 offset = 17;
		u8 ea_field = calc_EA_field(&ctx->inst.dest);
		if (ctx->inst.dest.mem_disp & 1)
		{
			u8 transfer_penalty = 8; // (2 * 4)
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea + %up)"), offset, ea_field, transfer_penalty);
			return offset + ea_field + transfer_penalty;
		}
		else
		{
			ctx->formula = str8_fmt(ctx->arena, STR8_LIT("(%u + %uea)"), offset, ea_field);
			return  offset + ea_field;
		}
	}
}

profilerFuncPtr profilerFuncPtrTable[256] = 
{    
	[0x00] = prof_modrm_common,
    [0x01] = prof_modrm_common,
    [0x02] = prof_modrm_common,
    [0x03] = prof_modrm_common,
    [0x08] = prof_modrm_common,
    [0x09] = prof_modrm_common,
    [0x0A] = prof_modrm_common,
    [0x0B] = prof_modrm_common,
    [0x0F] = prof_opcode_not_used,
    [0x10] = prof_modrm_common,
    [0x11] = prof_modrm_common,
    [0x12] = prof_modrm_common,
    [0x13] = prof_modrm_common,
    [0x18] = prof_modrm_common,
    [0x19] = prof_modrm_common,
    [0x1A] = prof_modrm_common,
    [0x1B] = prof_modrm_common,
    [0x20] = prof_modrm_common,
    [0x21] = prof_modrm_common,
    [0x22] = prof_modrm_common,
    [0x23] = prof_modrm_common,
    [0x28] = prof_modrm_common,
    [0x29] = prof_modrm_common,
    [0x2A] = prof_modrm_common,
    [0x2B] = prof_modrm_common,
    [0x30] = prof_modrm_common,
    [0x31] = prof_modrm_common,
    [0x32] = prof_modrm_common,
    [0x33] = prof_modrm_common,
    [0x38] = prof_modrm_common,
    [0x39] = prof_modrm_common,
    [0x3A] = prof_modrm_common,
    [0x3B] = prof_modrm_common,
    [0x80] = prof_imm_to_rm,
    [0x81] = prof_imm_to_rm,
    [0x82] = prof_imm_to_rm,
    [0x83] = prof_imm_to_rm,
	[0x84] = prof_modrm_test_xchg_mov,
    [0x85] = prof_modrm_test_xchg_mov,
    [0x86] = prof_modrm_test_xchg_mov,
    [0x87] = prof_modrm_test_xchg_mov,
    [0x88] = prof_modrm_test_xchg_mov,
    [0x89] = prof_modrm_test_xchg_mov,
    [0x8A] = prof_modrm_test_xchg_mov,
    [0x8B] = prof_modrm_test_xchg_mov,

    [0xB0] = prof_mov_imm_to_reg,
    [0xB1] = prof_mov_imm_to_reg,
    [0xB2] = prof_mov_imm_to_reg,
    [0xB3] = prof_mov_imm_to_reg,
    [0xB4] = prof_mov_imm_to_reg,
    [0xB5] = prof_mov_imm_to_reg,
    [0xB6] = prof_mov_imm_to_reg,
    [0xB7] = prof_mov_imm_to_reg,
    [0xB8] = prof_mov_imm_to_reg,
    [0xB9] = prof_mov_imm_to_reg,
    [0xBA] = prof_mov_imm_to_reg,
    [0xBB] = prof_mov_imm_to_reg,
    [0xBC] = prof_mov_imm_to_reg,
    [0xBD] = prof_mov_imm_to_reg,
    [0xBE] = prof_mov_imm_to_reg,
    [0xBF] = prof_mov_imm_to_reg,
};