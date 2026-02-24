#ifndef EXECUTE_INSTRUCTION_H
#define EXECUTE_INSTRUCTION_H

#include "common.h"
#include "decoder/decoder.h"

void execute_instruction(Arena *arena, s32 fd, Cpu cpu, Instruction inst);
void print_final_regs(Arena *arena, s32 fd, u16 *regs);

#endif