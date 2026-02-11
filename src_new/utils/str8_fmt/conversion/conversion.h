#ifndef CONVERSION_H
#define CONVERSION_H

#include "../str8_fmt.h"

void handle_int(Arena *arena, va_list args, t_fmt_opt opt);
void handle_x(Arena *arena, va_list args, t_fmt_opt opt);
void handle_upx(Arena *arena, va_list args, t_fmt_opt opt);
void handle_ptr(Arena *arena, va_list args, t_fmt_opt opt);
void handle_string8(Arena *arena, va_list args, t_fmt_opt opt);
void handle_char(Arena *arena, va_list args, t_fmt_opt opt);
void handle_percent_literal(Arena *arena, va_list args, t_fmt_opt opt);
void apply_padding(Arena *arena, String8 s, t_fmt_opt opt);

#endif