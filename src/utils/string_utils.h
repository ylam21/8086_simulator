#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "../common.h"

s64 string8tos64(String8 str);
s64	is_whitespace(u8 c);
u8 is_space(u8 c);
u8 is_alpha(u8 c);
u8 is_num(u8 c);
u8 is_alnum(u8 c);


#endif