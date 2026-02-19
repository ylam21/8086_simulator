#ifndef OPCODES_H
#define OPCODES_H

#include <stddef.h>
#include "../arena/arena.h"
#include "../common.h"

typedef struct
{
    u8 execute;
    Arena *arena;
    s32 fd;
    u8 *b;
    u8 seg_prefix;
    u32 current_ip;
    u16 regs[14]; // Array where we store the state of all 14 registers, 16-bit wide each
} t_ctx;


typedef u8 (*func_ptr)(t_ctx *ctx); 

extern func_ptr opcode_table[256];

#endif