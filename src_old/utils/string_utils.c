#include "string_utils.h"

u8 is_space(u8 c)
{
    return (c == ' ');
}

u8 is_alpha(u8 c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

u8 is_num(u8 c)
{
    return ('0' <= c && c <= '9');
}

u8 is_alnum(u8 c)
{
    return (is_alpha(c) || is_num(c));
}

u8	is_whitespace(u8 c)
{
	return (c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'
		|| c == ' ');
}

static u64	get_start_pos(String8 s)
{
	u64	pos;

	pos = 0;
	while (pos < s.size)
	{
		if (!is_space(s.str[pos]))
			return (pos);
		pos += 1;
	}
	return (pos);
}


s16 string8_decimal_to_s16(String8 s)
{
	u64	start_pos;
	u64	pos;
	s64	sign;
	s64	acc;

	start_pos = get_start_pos(s);
	if (start_pos == s.size)
		return (0);
	pos = start_pos;
	sign = 0;
	if (s.str[pos] == '-' || s.str[pos] == '+')
	{
		if (s.str[pos] == '-')
			sign = 1;
		pos++;
	}
	acc = 0;
	while (pos < s.size && is_num(s.str[pos]))
	{
		acc = acc * 10 + s.str[pos] - '0';
		pos++;
	}
	if (sign)
		return (acc * -1);
	return ((s16)acc);
}

static u8 to_lower(u8 c)
{
	if ('A' <= c && c <= 'Z')
		return (c + 32);
	return (c);
}


u16 hex_to_u16(u8 c) {
    if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
    u8 c_lower = to_lower(c);
    if (c_lower >= 'a' && c_lower <= 'f')
	{
		return 10 + (c_lower - 'a');
	}
    return 0xFFFF;
}

s32 string8_hex_to_s16(String8 s)
{
	u64 start_pos;

	start_pos = get_start_pos(s);
	start_pos += 2; // skip the '0x' prefix

	if (start_pos == s.size)
	{
		return 0;
	}

	u8 pos = start_pos;
	u16 acc = 0;
	while (pos < s.size && is_alnum(s.str[pos]))
	{
		u16 val = hex_to_u16(s.str[pos]);
		acc = (acc << 4) | val;
		pos += 1;
	}

	return (s16)acc;
}