#ifndef CONVERSION_H
#define CONVERSION_H

#include "../strjoin_fmt.h"

u8 *handle_int(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_x(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_upx(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_ptr(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_string(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_char(Arena *a, va_list args, t_fmt_opt opt);
u8 *handle_percent_literal(Arena *a, va_list args, t_fmt_opt opt);
u8 *apply_padding(Arena *a, u8 *str, t_fmt_opt opt);

#endif