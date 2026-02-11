#ifndef STRJOIN_FMT_H
#define STRJOIN_FMT_H


#include "../../common.h"

typedef struct
{
    u8 padding_char;
    s32 width;
    u8 left_align;
    u8 is_conditional; // special handling for '?' prefix specifier
} t_fmt_opt;

#include "conversion/conversion.h"

#define CHAR_SPACE ' '

typedef void (*fmt_handler_t)(Arena *arena, va_list args, t_fmt_opt opt);

extern fmt_handler_t specifier_table[256];

String8 str8_fmt(Arena *arena, String8 fmt, ...);

#endif