#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

typedef String8 (*translateOperand)(Arena *arena, Operand op, u8 W);

void print_instruction(Arena *arena, s32 fd, Instruction inst);
u8 is_shift(String8 mnemonic);
String8 translateOpNone(Arena *arena, Operand op, u8 W);
String8 translateOpRegister(Arena *arena, Operand op, u8 W);
String8 translateOpRegisterDx(Arena *arena, Operand op, u8 W);
String8 translateOpRegisterCl(Arena *arena, Operand op, u8 W);
String8 translateOpSegRegister(Arena *arena, Operand op, u8 W);
String8 translateOpImmediate(Arena *arena, Operand op, u8 W);
String8 translateOpMemory(Arena *arena, Operand op, u8 W);
String8 translateOpMemoryDir(Arena *arena, Operand op, u8 W);
String8 translateOpIpRelative(Arena *arena, Operand op, u8 W);

#endif