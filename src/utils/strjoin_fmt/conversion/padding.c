#include "conversion.h"

u8 *apply_padding(Arena *arena, u8 *str, t_fmt_opt opt)
{
    u64 len = strlen((char *)str);

    if (len >= (u64)opt.width)
    {
        u8 *result = arena_push(arena, len);
        if (!result) return NULL;
        memcpy(result, str, len);
        return result;
    }

    s32 total_padding = opt.width - len;
    
    u8 *padded = arena_push(arena, opt.width); 
    if (!padded)
    {
        return NULL;
    }

    if (opt.left_align)
    {   
        /* <TEXT;SPACES> */
        memcpy(padded, str, len);
        memset(padded + len, CHAR_SPACE, total_padding);
    }
    else
    {
        /* <SPACES/ZEROS;TEXT> */
        memset(padded, opt.padding_char, total_padding);
        memcpy(padded + total_padding, str, len);
    }

    return padded;
}