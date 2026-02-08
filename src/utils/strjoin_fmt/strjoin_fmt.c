#include "strjoin_fmt.h"

static t_fmt_opt parse_options(u8 **fmt)
{
    t_fmt_opt opt = { .padding_char = CHAR_SPACE, .width = 0, .left_align = 0};
    u8 *f = *fmt;

    while (*f == '-' || *f == '0')
    {
        if (*f == '-')
        {
            opt.left_align = 1;
        }
        else if (*f == '0')
        {
            opt.padding_char = '0';
        }
        f++;
    }

    while (*f >= '0' && *f <= '9')
    {
        opt.width = ((opt.width << 3) + ( opt.width << 1)) + (*f - '0');
        f++;
    }

    *fmt = f;
    return opt;
}

u8 *strjoin_fmt(Arena *arena, u8 *fmt, ...)
{
    u8 *result = (u8 *)arena->buffer + arena->pos;

    va_list args;
    va_start(args, fmt);

    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;
            t_fmt_opt opt = parse_options(&fmt);

            fmt_handler_t handler = specifier_table[*fmt];
            if (handler)
            {
                handler(arena, args, opt);
            }
            else if (*fmt)
            {
                u8 *c = arena_push(arena, 1);
                if (!c)
                {
                    return NULL;
                }
                *c = *fmt;
            }
        }
        else
        {
            u8 *c = arena_push(arena, 1u);
            if (!c)
            {
                return NULL;
            }
            *c = *fmt;
        }
        fmt++;
    }

    va_end(args);
    u8 *c = arena_push(arena, 1u);
    if (!c)
    {
        return NULL;
    }
    *c = '\0';
    return result;
}
