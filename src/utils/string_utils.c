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

static s64	get_start_idx(u8 *s)
{
	s64	idx;

	idx = 0;
	while (s[idx] != 0)
	{
		if (!is_space(s[idx]))
			return (idx);
		idx++;
	}
	return (-1);
}

s64 string8tos64(String8 s)
{
	s64	start_pos;
	s64	pos;
	s64	sign;
	s64	acc;

	start_pos = get_start_idx(s.str);
	if (start_pos == -1)
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
	while (s.str[pos] != 0 && is_num(s.str[pos]))
	{
		acc = acc * 10 + s.str[pos] - '0';
		pos++;
	}
	if (sign)
		return (acc * -1);
	return (acc);
}
