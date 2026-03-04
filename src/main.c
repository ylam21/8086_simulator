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

String8 create_filename_out_from_listing_examples(Arena *arena, char *s)
{
    u8 temp[10];
    u8 temp_pos = 0;
    u32 pos = 0;
    while (s[pos])
    {
        if (isdigit(s[pos]) && temp_pos < 10)
        {
            temp[temp_pos] = s[pos];
            temp_pos += 1;
        }
        pos += 1;
    }

    String8 res = str8_fmt(arena, STR8_LIT("%s_out.asm"), (String8){.size = temp_pos, .str = temp});
    u8 *null_term = arena_push_packed(arena, 1);
    *null_term = '\0';
    return res;
}

void print_usage(void)
{
    fprintf(stdout, "Usage examples:\n");
    fprintf(stdout, "Execution mode: %s <filename> -exec\n", PROGRAM_PATH);
    fprintf(stdout, "Execution mode with dump: %s <filename> -exec -dump\n", PROGRAM_PATH);
    fprintf(stdout, "Disassembly mode: %s <filename> -disasm\n", PROGRAM_PATH);
    fprintf(stdout, "Profiler mode: %s filename> -showclocks\n", PROGRAM_PATH);
    fprintf(stdout, "Profiler mode with more explanation: %s <filename> -explainclocks\n", PROGRAM_PATH);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return (EXIT_FAILURE);
    }

    u32 StartFlags = 0;

    u8 idx = 0;
    char *arg = argv[idx];
    char *filename = NULL;
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
        else if (strcmp(arg, "-help") == 0)
        {
            print_usage();
            return (EXIT_SUCCESS);
        }

        idx += 1;
        arg = argv[idx];
    }

    if (StartFlags == 0)
    {
        fprintf(stdout, "Error: No flags provided. Provide -help flag for more context.\n");
        return (EXIT_SUCCESS);
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

    Arena *arena = arena_create(MegaByte(1));
    if (!arena)
    {
        fprintf(stderr, "Error: Cannot create an arena\n");
        return (EXIT_FAILURE);
    }

    String8 filename_out = create_filename_out_from_listing_examples(arena, filename);
    // filename_out is null terminated
    s32 fd_out = open((const char *)filename_out.str, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd_out == -1)
    {
        fprintf(stderr, "Error: Cannot create a file\n");
        return (EXIT_FAILURE);
    }

    if (read_bytes == 0)
    {
        fprintf(stderr, "Nothing to decode\n");
        return (EXIT_SUCCESS);
    }

    String8 header = STR8_LIT("bits 16\n\n");
    s32 written = write(fd_out, header.str, header.size);
    if (written == -1) return EXIT_FAILURE;

    
    if (StartFlags & StartFlagExecute)
    {
        execute_8086(arena, buffer, (u64)read_bytes, fd_out, StartFlags);
    }
    else if (StartFlags & StartFlagShowClocks)
    {
        run_8086_profiler(arena, buffer, (u64)read_bytes, fd_out, StartFlagShowClocks);
    }
    else if (StartFlags & StartFlagExplainClocks)
    {
        run_8086_profiler(arena, buffer, (u64)read_bytes, fd_out, StartFlagExplainClocks);
    }
    else
    {
        disasm_8086(arena, buffer, (u64)read_bytes, fd_out);
    }

    fprintf(stdout, "Written output to: \"%s\"\n", (const char *)filename_out.str);
    #ifdef DEBUG
       arena_destroy(arena);
    #endif
    return (EXIT_SUCCESS);
}