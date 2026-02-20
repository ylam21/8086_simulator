#ifndef OPCODES_H
#define OPCODES_H

#include <stddef.h>
#include "../arena/arena.h"
#include "../common.h"
#include "decoder.h"

typedef struct
{
    Arena *arena;
    u8 *b;
    u8 seg_prefix;
    u32 current_ip;
} t_ctx; // TODO: rename t_ctx to Ctx



typedef Instruction (*func_ptr)(t_ctx *ctx); 

extern func_ptr opcode_table[256];

#endif