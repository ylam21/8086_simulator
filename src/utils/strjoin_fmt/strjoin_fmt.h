#ifndef STRJOIN_FMT_H
#define STRJOIN_FMT_H

#include <stdarg.h>
#include "../../common.h"
#include "../../arena.h"

typedef struct
{
    u8 padding_char;
    s32 width;
    u8 left_align;
} t_fmt_opt;

#define CHAR_SPACE ' '

typedef u8* (*fmt_handler_t)(Arena *arena, va_list args, t_fmt_opt opt);

extern fmt_handler_t specifier_table[256];

u8 *strjoin_fmt(Arena *arena, u8 *fmt, ...);

#endif