#ifndef PROFILER_TABLE_H
#define PROFILER_TABLE_H

u8 prof_opcode_not_used(Instruction inst);
u8 prof_mov_imm_to_reg(Instruction inst);
u8 prof_modrm_common(Instruction inst);
u8 prof_modrm_test_xchg_mov(Instruction inst);
u8 prof_modrm(Instruction *inst);
extern profilerFuncPtr profilerFuncPtrTable[256];

enum 
{
	RM_BX_SI,
	RM_BX_DI,
	RM_BP_SI,
	RM_BP_DI,
	RM_SI,
	RM_DI,
	RM_BP,
	RM_BX,
	RM_EAC_CNT,
} effAddCalcType;

extern u8 match_EA_component_with_clocks[RM_EAC_CNT];

#endif