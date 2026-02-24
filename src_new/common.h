#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef struct Arena Arena; 
typedef struct String8 String8;
typedef struct Cpu Cpu;

struct String8
{
    u64 size;
    u8 *str;
};

struct Cpu
{
    u16 *regs;
};


#define STR8_LIT(s) (String8){sizeof(s) - 1, (u8*)(s)}

#include "arena/arena.h"
#include "utils/str8_fmt/str8_fmt.h"

#define PROGRAM_PATH "bin/simulate8086"

#endif