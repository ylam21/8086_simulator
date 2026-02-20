#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "common.h" 
#include "utils/io_utils.h"
#include "decoder/decoder.h"
#include "decoder/opcodes.h"

#define MASK_EXECUTE 0x1
#define MASK_DISASM 0x2

enum start_flags
{
    StartFlagExecute = 0x1,
};


void print_instruction(Instruction inst)
{
    Instruction = {0};
}



u8 is_op_prefix(u8 opcode)
{
    return (opcode == 0x26 ||\
            opcode == 0x2E ||\
            opcode == 0x36 ||\
            opcode == 0x3E);
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
        arg = argv[idx];
        if (strcmp(arg, "-exec") == 0)
        {
            StartFlags = 0;
            StartFlags |= StartFlagExecute;
        }
        idx += 1;
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
    u8 offset = 0;
    t_ctx ctx = 
    {
        .arena = arena,
        .b = buffer,
        .current_ip = offset,
        .seg_prefix = 0xFF,
    };
    
    u64 ip = 0;
    u64 end = (u64)read_bytes;
    while (ip < end)
    {
        ctx.b = &buffer[ip];
        opcode = ctx.b[0];
        u8 is_prefix = is_op_prefix(opcode);
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);

        print_instruction(inst);

        if (StartFlags & StartFlagExecute)
        {
            execute_instruction();
        }

    }

    close(fd_out);
    printf("Output written to: \"%s\"\n",filename_out);
    arena_destroy(arena);
    return (EXIT_SUCCESS);
}