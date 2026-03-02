u8 isTypeReg(OperandType t)
{
	return (t == OP_REGISTER || t == OP_REGISTER_CL || t == OP_REGISTER_DX);
}

u8 prof_mov_imm_to_reg(Instruction inst)
{
	(void)inst;
	return 4;
}

u8 prof_opcode_not_used(Instruction inst)
{
	(void)inst;
	return 0;
}

u8 prof_modrm_common(Instruction inst) {return prof_modrm(&inst);}
u8 prof_modrm_test_xchg_mov(Instruction inst) {return prof_modrm(&inst);}

u8 prof_modrm(Instruction *inst)
{
	OperandType dType = inst->dest.type;
	OperandType sType = inst->src.type;
	
	if (isTypeReg(dType) && isTypeReg(sType))
	{
		return 2;
	}
	else if (isTypeReg(dType) && (sType == OP_MEMORY || sType == OP_MEMORY_DIR))
	{
		u8 clocks = 8;
		if (sType == OP_MEMORY_DIR)
		{
			return clocks + 6;
		}
		clocks += match_EA_component_with_clocks[inst->src.mem_base_reg];
		if (inst->src.mem_disp != 0)
		{
			return clocks + 4;
		}
		else
		{
			return clocks;
		}
	}
	else if ((dType == OP_MEMORY || dType == OP_MEMORY_DIR) && isTypeReg(sType))
	{
		u8 clocks = 9;
		if (dType == OP_MEMORY_DIR)
		{
			return clocks + 6;
		}
		clocks += match_EA_component_with_clocks[inst->dest.mem_base_reg];
		if (inst->dest.mem_disp != 0)
		{
			return clocks + 4;
		}
		else
		{
			return clocks;
		}
	}
	return 0;
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