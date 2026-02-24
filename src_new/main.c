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
    
    u8 opcode;
    u64 ip = 0;
    t_ctx ctx = 
    {
        .b = buffer,
        .current_ip = ip,
        .seg_prefix = 0xFF,
    };

    u16 regs[14] = {0};
    Cpu cpu = {.regs = regs};

    u64 end = (u64)read_bytes;
    while (ip < end)
    {
        ctx.b = &buffer[ip];
        ctx.current_ip = ip;
        opcode = ctx.b[0];
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);

        print_instruction(arena, fd_out, inst);

        if (StartFlags & StartFlagExecute)
        {
            execute_instruction(arena, fd_out, cpu, inst);
        }
        arena_reset(arena);
        write(fd_out, "\n", 1);
        ip += inst.size;
    }

    if (StartFlags & StartFlagExecute)
    {
        print_final_regs(arena, fd_out, cpu.regs);
    }

    close(fd_out);
    fprintf(stdout, "Output written to: \"%s\"\n", filename_out);
    arena_destroy(arena);
    return (EXIT_SUCCESS);
}