#ifndef BASE_DEFS_H
#define BASE_DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <ctype.h>

#define PROGRAM_PATH "./simulate8086"
#define MegaByte(d) ((d) << 20)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define CHAR_SPACE ' '

#define AX_IDX 0 
#define CX_IDX 1
#define DX_IDX 2
#define BX_IDX 3
#define SP_IDX 4
#define BP_IDX 5
#define SI_IDX 6
#define DI_IDX 7
#define IP_IDX 12
#define FLAGS_IDX 13

#define MASK_EXECUTE 0
#define MASK_DISASM 1
#define MASK_DUMP 2
#define MASK_SHOWCLOCKS 3
#define MASK_EXPLAINCLOCKS 4

#define INST_MOV STR8_LIT("mov")
#define INST_ADD STR8_LIT("add")
#define INST_SUB STR8_LIT("sub")
#define INST_CMP STR8_LIT("cmp")
#define INST_JNZ STR8_LIT("jnz")
#define INST_LOOP STR8_LIT("loop")

enum start_flags
{
    StartFlagExecute = 0x1,
    StartFlagDisasm = 0x2,
    StartFlagDump = 0x4,
    StartFlagShowClocks = 0x8,
    StartFlagExplainClocks = 0x10,
};

typedef struct Cpu Cpu;
struct Cpu
{
    u16 regs[14];
    u8 Memory[MegaByte(1)];
};

typedef struct t_ctx t_ctx;
struct t_ctx
{
    u8 *b;
    u8 seg_prefix;
    u16 ip;
}; 

#endif