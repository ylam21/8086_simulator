#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "common.h" 
#include "utils/io_utils.h"
#include "decoder/decoder.h"
#include "decoder/opcodes.h"
#include "print_instruction.h"
#include "execute_instruction.h"

#define MASK_EXECUTE 0x1
#define MASK_DISASM 0x2

enum start_flags
{
    StartFlagExecute = 0x1,
    StartFlagDisasm = 0x2,
};

static void execute_8086(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd)
{
    u8 opcode;

    t_ctx ctx = 
    {
        .b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };

    u16 regs[14] = {0};
    Cpu cpu = {.regs = regs};
    u16 regsStateOld[14] = {0}; // NOTE: buffer where we store the old state of cpu.regs

    while (ctx.ip < read_bytes)
    {
        memcpy(regsStateOld, cpu.regs, 14 * sizeof(u16)); // NOTE: save the old state of cpu.regs
        ctx.b = &buffer[ctx.ip];
        opcode = ctx.b[0];
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);
        ctx.ip += inst.size;
        cpu.regs[12] = ctx.ip;
        print_instruction(arena, fd, inst);
        execute_instruction(arena, fd, cpu, inst, regsStateOld, &ctx);
        write(fd, "\n", 1);
        arena_reset(arena);
    }
    print_final_regs(arena, fd, cpu.regs);
}

static void disasm_8086(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd)
{
    u8 opcode;
    t_ctx ctx = 
    {
        .b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };
    while (ctx.ip < read_bytes)
    {
        ctx.b = &buffer[ctx.ip];
        opcode = ctx.b[0];
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);
        ctx.ip += inst.size;
        print_instruction(arena, fd, inst);
        write(fd, "\n", 1);
        arena_reset(arena);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename> <start_flag>\n", PROGRAM_PATH);
        return (EXIT_FAILURE);
    }

    u8 StartFlags = 0;

    u8 idx = 0;
    char *arg = argv[idx];
    while (arg)
    {
        if (strcmp(arg, "-exec") == 0)
        {
            StartFlags = 0;
            StartFlags |= StartFlagExecute;
        }
        idx += 1;
        arg = argv[idx];
    }

    char *filename = argv[1];
    s32 fd_in = open(filename, O_RDONLY);
    if (fd_in == -1)
    {
        perror(filename);
        return (EXIT_FAILURE);
    }

    u8 buffer[1024];
    s64 read_bytes = read(fd_in, buffer, 1024);
    close(fd_in);
    if (read_bytes == -1)
    {
        fprintf(stderr, "Error: Cannot read from %s\n", filename);
        return (EXIT_FAILURE);
    }

    fprintf(stdout, "Read %lu bytes from: \"%s\"\n", read_bytes, filename);

    char *filename_out = "out.asm";
    s32 fd_out = open(filename_out, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd_out == -1)
    {
        fprintf(stderr, "Error: Cannot create a file\n");
        return (EXIT_FAILURE);
    }

    if (read_bytes == 0)
    {
        fprintf(stderr, "Nothing to decode\n");
        close(fd_out);
        return (EXIT_SUCCESS);
    }

    String8 header = STR8_LIT("bits 16\n\n");
    write(fd_out, header.str, header.size);

    Arena *arena = arena_create(1024);
    if (!arena)
    {
        fprintf(stderr, "Error: Cannot create an arena\n");
        return (EXIT_FAILURE);
    }
    
    if (StartFlags & StartFlagExecute)
    {
        execute_8086(arena, buffer, (u64)read_bytes, fd_out);
    }
    else
    {
        disasm_8086(arena, buffer, (u64)read_bytes, fd_out);
    }

    close(fd_out);
    fprintf(stdout, "Output written to: \"%s\"\n", filename_out);
    arena_destroy(arena);
    return (EXIT_SUCCESS);
}