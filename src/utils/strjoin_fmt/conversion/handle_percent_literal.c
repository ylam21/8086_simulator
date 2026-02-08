#include "conversion.h"

u8 *handle_percent_literal(Arena *arena, va_list args, t_fmt_opt opt)
{
    (void)args;
    (void)opt;

    u8 *str = arena_push(arena, 1);
    if (!str)
    {
        return NULL;
    }
    
    str[0] = '%';

    return str;
}