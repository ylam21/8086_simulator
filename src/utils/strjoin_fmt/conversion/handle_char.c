#include "conversion.h"

u8 *handle_char(Arena *arena, va_list args, t_fmt_opt opt)
{
    u8 c;

    c = (u8)va_arg(args, s32);
    u8 *str = arena_push(arena, 1);
    if (!str)
    {
        return NULL;
    }

    str[0] = c;

    if (opt.width > 0)
    {
        return apply_padding(arena, str, opt);
    }
    else
    {
        return str;
    }
}