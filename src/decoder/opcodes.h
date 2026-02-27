#ifndef OPCODES_H
#define OPCODES_H

#include <stddef.h>
#include "../arena/arena.h"
#include "../common.h"
#include "decoder.h"

typedef struct t_ctx t_ctx;
struct t_ctx
{
    u8 *b;
    u8 seg_prefix;
    u16 ip;
}; 

typedef Instruction (*func_ptr)(t_ctx *ctx); 

extern func_ptr opcode_table[256];

#endif