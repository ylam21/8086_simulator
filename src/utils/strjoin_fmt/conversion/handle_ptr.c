#include "conversion.h"

static u8 *handle_hex_ptr(Arena *arena, va_list args, t_fmt_opt opt, u8 table[16])
{
    void *ptr = va_arg(args, void *);
    u8 buffer[20];
    u32 digit_count = 0;
    u64 val = (u64)ptr;
    u8 *str;

    if (!ptr)
    {
        u8 size = strlen("(nil)");
        str = arena_push(arena, size);
        if (!str)
        {
            return NULL;
        }
        memcpy(str, "(nil)", size);
        str[size] = '\0';
        return str;
    }

    if (val == 0)
    {
        buffer[digit_count] = '0';
        digit_count++;
    }
    else
    {
        while (val)
        {
            buffer[digit_count++] = table[val & 0xF];
            val >>= 4;
        }
    }

    str = arena_push(arena, digit_count + 2);
    if (!str)
    {
        return NULL;
    }
    
    str[0] = '0';
    str[1] = 'x';

    u32 i = 0;
    while (i < digit_count)
    {
        str[2 + i] = buffer[digit_count - 1 - i];
        i++;
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

u8 *handle_ptr(Arena *arena, va_list args, t_fmt_opt opt)
{
    return handle_hex_ptr(arena, args, opt, (u8 *)"0123456789abcdef");
}