#ifndef OPCODES_H
#define OPCODES_H

typedef Instruction (*func_ptr)(t_ctx *ctx); 

extern func_ptr opcode_table[256];

#endif