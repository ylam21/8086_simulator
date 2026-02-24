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

String8 table_reg_w_zero[8] =
{
    STR8_LIT("al"),
    STR8_LIT("cl"),
    STR8_LIT("dl"),
    STR8_LIT("bl"),
    STR8_LIT("ah"), 
    STR8_LIT("ch"), 
    STR8_LIT("dh"),
    STR8_LIT("bh"),
};

String8 table_reg_w_one[8] =
{
    STR8_LIT("ax"),
    STR8_LIT("cx"),
    STR8_LIT("dx"),
    STR8_LIT("bx"),
    STR8_LIT("sp"), 
    STR8_LIT("bp"), 
    STR8_LIT("si"),
    STR8_LIT("di"),
};

String8 table_mem_address_calc[8] = 
{
    STR8_LIT("bx + si"),
    STR8_LIT("bx + di"), 
    STR8_LIT("bp + si"), 
    STR8_LIT("bp + di"), 
    STR8_LIT("si"),
    STR8_LIT("di"),
    STR8_LIT("bp"),
    STR8_LIT("bx"),
};

String8 table_sreg[4] =
{
    STR8_LIT("es"),
    STR8_LIT("cs"),
    STR8_LIT("ss"),
    STR8_LIT("ds"),
};

enum start_flags
{
    StartFlagExecute = 0x1,
};


// TODO: implement jump table
String8 translate_operand(Arena *arena, Operand op, u8 W)
{
    if (OP_NONE)
    {
        return (String8){.size = 0};
    }
    else if (OP_REGISTER)
    {
        if (W == 0)
        {
            return table_reg_w_zero[op.reg_idx];
        }
        else
        {
            return table_reg_w_one[op.reg_idx];
        }
    }
    else if (OP_SREG)
    {
        return table_sreg[op.reg_idx];
    }
    else if (OP_IMMEDIATE)
    {
        String8 prefix;
        if (W == 0)
        {
            prefix = STR8_LIT("byte");
        }
        else
        {
            prefix = STR8_LIT("word");
        }
        return str8_fmt(arena, STR8_LIT("%s %d"), prefix, op.immediate_val);
    }
    else if (OP_MEMORY)
    {
        if (op.mem_disp != 0)
        {
            return str8_fmt(arena, STR8_LIT("[%s + %u]"), table_mem_address_calc[op.mem_base_reg], op.mem_disp);
        }
        else
        {
            return str8_fmt(arena, STR8_LIT("[%s]"), table_mem_address_calc[op.mem_base_reg]);
        }
    }
    else
    {
        return (String8){.size = 0};
    }
}

void print_instruction(Arena *arena, s32 fd, Instruction inst)
{
    String8 dest = translate_operand(arena, inst.dest, inst.w_bit);
    String8 src = translate_operand(arena, inst.src, inst.w_bit);
    String8 res;
    
    if (src.size != 0)
    {
        res = str8_fmt(arena, STR8_LIT("%s %s, %s\n"), inst.mnemonic, dest, src);
    }
    else
    {
        res = str8_fmt(arena, STR8_LIT("%s %s\n"), inst.mnemonic, dest);
    }
    write(fd, res.str, res.size);
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
    
    u64 end = (u64)read_bytes;
    while (ip < end)
    {
        ctx.b = &buffer[ip];
        ctx.current_ip = ip;
        opcode = ctx.b[0];
        u8 is_prefix = is_op_prefix(opcode);
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);

        (void)is_prefix;
        print_instruction(arena, fd_out, inst);

        if (StartFlags & StartFlagExecute)
        {
            // execute_instruction();
        }
        ip += inst.size;
        arena_reset(arena);
    }

    close(fd_out);
    fprintf(stdout, "Output written to: \"%s\"\n", filename_out);
    arena_destroy(arena);
    return (EXIT_SUCCESS);
}