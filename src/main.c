#include "base/base_inc.h"
#include "decoder/decoder_inc.h"
#include "profiler/profiler_inc.h"
#include "disassembler/disassembler_inc.h"
#include "executor/executor_inc.h"

#include "base/base_inc.c"
#include "decoder/decoder_inc.c"
#include "profiler/profiler_inc.c"
#include "disassembler/disassembler_inc.c"
#include "executor/executor_inc.c"

void write_memory(u8 *memory, u64 size)
{
    const char *filename = "sim86_memory.data";
    s32 fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1)
    {
        perror(filename);
        return;
    }
    s32 written = write(fd, memory, size);
    if (written == -1) return;
    fprintf(stdout, "Written dump data to: \"%s\"\n", filename);
}

void execute_8086(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u32 startFlags)
{
    u8 opcode;

    t_ctx ctx = 
    {
        .b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };

    Cpu cpu =
    {
        .regs = {0},
        .Memory = {0},
    };

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
        execute_instruction(arena, fd, &cpu, inst, regsStateOld, &ctx);
        s32 written = write(fd, "\n", 1);
        if (written == -1) return;
        
        arena_reset(arena);
    }
    print_final_regs(arena, fd, cpu.regs);

    if ((startFlags >> MASK_DUMP) & 1)
    {
        write_memory(cpu.Memory, MegaByte(1));
    }
}

void disasm_8086(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd)
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
        s32 written = write(fd, "\n", 1);
        if (written == -1) return;
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

    u32 StartFlags = 0;

    u8 idx = 0;
    char *arg = argv[idx];
    const char *filename = NULL;
    while (arg)
    {
        if (strcmp(arg, "-exec") == 0)
        {
            StartFlags |= StartFlagExecute;
        }
        else if (strcmp(arg, "-dump") == 0)
        {
            StartFlags |= StartFlagDump;
        }
        else if (strcmp(arg, "-disasm") == 0)
        {
            StartFlags |= StartFlagDisasm;
        }
        else if (strcmp(arg, "-showclocks") == 0)
        {
            StartFlags |= StartFlagShowClocks;
        }
        else if (strcmp(arg, "-explainclocks") == 0)
        {
            StartFlags |= StartFlagExplainClocks;
        }
        else if (arg[0] != '-')
        {
            filename = arg;
        }

        idx += 1;
        arg = argv[idx];
    }

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
    s32 written = write(fd_out, header.str, header.size);
    if (written == -1) return EXIT_FAILURE;

    Arena *arena = arena_create(1024);
    if (!arena)
    {
        fprintf(stderr, "Error: Cannot create an arena\n");
        return (EXIT_FAILURE);
    }
    
    if (StartFlags & StartFlagExecute)
    {
        execute_8086(arena, buffer, (u64)read_bytes, fd_out, StartFlags);
    }
    else if (StartFlags & StartFlagShowClocks || StartFlags & StartFlagExplainClocks)
    {
        run_8086_profiler(arena, buffer, (u64)read_bytes, fd_out, StartFlags);
    }
    else
    {
        disasm_8086(arena, buffer, (u64)read_bytes, fd_out);
    }

    close(fd_out);
    fprintf(stdout, "Written output to: \"%s\"\n", filename_out);
    arena_destroy(arena);
    return (EXIT_SUCCESS);
}