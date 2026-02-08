#ifndef STR8_FMT_H
#define STR8_FMT_H

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

typedef void (*fmt_handler_t)(Arena *arena, va_list args, t_fmt_opt opt);

extern fmt_handler_t specifier_table[256];

String8 str8_fmt(Arena *arena, u8 *fmt, ...);

#endif