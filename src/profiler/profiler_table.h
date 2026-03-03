#ifndef PROFILER_TABLE_H
#define PROFILER_TABLE_H

u8 prof_opcode_not_used(prof_ctx *ctx);
u8 prof_mov_imm_to_reg(prof_ctx *ctx);
u8 prof_modrm_common(prof_ctx *ctx);
u8 prof_modrm_test_xchg_mov(prof_ctx *ctx);
u8 prof_imm_to_rm(prof_ctx *ctx);
u8 prof_modrm(prof_ctx *ctx, u8 *offsets, u8 *transfer_penalty);
u8 calc_EA_field(Operand *op);

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

enum
{
	OFFSET_REG_TO_REG,
	OFFSET_MEM_TO_REG,
	OFFSET_REG_TO_MEM,
} offsets_prof_type;

extern u8 match_EA_component_with_clocks[RM_EAC_CNT];

#endif