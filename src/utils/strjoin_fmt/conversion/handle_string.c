#include "conversion.h"

u8 *handle_string(Arena *arena, va_list args, t_fmt_opt opt)
{
    (void)opt;
    u8 *str = va_arg(args, u8 *);

    if (!str)
    {
        str = (u8 *)"(null)";
    }

    u64 len = strlen((char *)str);
    u64 needed_size = len;

    u8 *result = arena_push(arena, needed_size);
    if (!result)
    {
        return NULL;
    }

    memcpy(result, str, len);

    return result;
}
