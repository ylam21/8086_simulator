#include "conversion.h"

static size_t i32len(s32 x)
{
	size_t len;

	len = (x <= 0);
	while (x)
	{
		len++;
		x /= 10;
	}
	return (len);
}

u8 *itoa(Arena *arena, s32 x)
{
	u64 len;
    s64 nb;
    u8 *result;

    len = i32len(x);
    result = arena_push(arena, len); 
    if (result == NULL)
    {
        return NULL;
    }

    nb = x;
    if (nb == 0)
    {
        result[0] = '0';
    }
    else if (nb < 0)
    {
        result[0] = '-';
        nb = -nb;
    }

    while (nb)
    {
        result[--len] = (nb % 10) + '0';
        nb /= 10;
    }
    return (result);
}
u8 *handle_int(Arena *arena, va_list args, t_fmt_opt opt)
{
    s32 val;
    u8 *str;

    val = va_arg(args, s32);
    str = itoa(arena, val);
    if (!str)
    {
        return NULL;
    }
    if (opt.width > 0)
    {
        return apply_padding(arena, str, opt);
    }
    else
    {
        return str;
    }
}