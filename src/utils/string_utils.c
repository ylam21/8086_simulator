#include "string_utils.h"

u8 is_space(u8 c)
{
    return (c == ' ');
}

u8 is_alpha(u8 c)
{
    return ('a' <= c && c <= 'z');
}

u8 is_num(u8 c)
{
    return ('0' <= c && c <= '9');
}

u8 is_alnum(u8 c)
{
    return (is_alpha(c) || is_num(c));
}

s64	is_whitespace(u8 c)
{
	return (c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'
		|| c == ' ');
}

static u64	get_start_idx(String8 s)
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

s64 string8tos64(String8 s)
{
	u64	start_pos;
	u64	pos;
	s64	sign;
	s64	acc;

	start_pos = get_start_idx(s);
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
	return (acc);
}
