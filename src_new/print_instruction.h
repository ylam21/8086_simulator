#ifndef PRINT_INSTRUCTION_H
#define PRINT_INSTRUCTION_H

#include "unistd.h"
#include "common.h"
#include "decoder/decoder.h"
#include "utils/io_utils.h"

void print_instruction(Arena *arena, s32 fd, Instruction inst);

#endif