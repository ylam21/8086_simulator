#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdarg.h>

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


struct String8
{
    u64 size;
    u8 *str;
};
#define STR8_LIT(s) (String8){ (u8*)(s), sizeof(s) - 1 }

#include "arena.h"
#include "utils/string_utils.h"
#include "utils/strjoin_fmt/strjoin_fmt.h"

#define PROGRAM_PATH "bin/simulate8086"
#define LIFE_ARENA_SIZE 1024*1024


#endif