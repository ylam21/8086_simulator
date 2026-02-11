#include <fcntl.h>
#include "common.h"
#include "simulate_8086.h"

static String8 read_file(Arena *arena, s32 fd)
{
    u64 len = (u64)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    u8 *data = arena_push(arena, len);
    if (!data)
    {
        return (String8){0};
    }
    
    s64 bytes_read = read(fd, data, len);
    if (bytes_read != (s64)len)
    {
        fprintf(stderr, "Error: Read unexpected amount of bytes\n");
        return (String8){0};
    }

    return (String8){ .str = data, .size = len};
}


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename.asm>\n", PROGRAM_PATH);
        return (EXIT_FAILURE);
    }

    
    u8 *filename_in = (u8 *)argv[1];
    s32 fd_in = open((char *)filename_in, O_RDONLY);
    if (fd_in == -1)
    {
        fprintf(stderr, "Error: Could not open %s\n", filename_in);
        return (EXIT_FAILURE);
    }
    
    u8 *filename_out = (u8 *)"out.txt";
    s32 fd_out = open((char *)filename_out, O_CREAT | O_TRUNC | O_WRONLY, 0777);
    if (fd_out == -1)
    {
        fprintf(stderr, "Error: Could not open %s\n", filename_out);
        close(fd_in);
        return (EXIT_FAILURE);
    }

    Arena *life_arena = arena_create(1024 * 1024);
    if (!life_arena)
    {
        close(fd_in);
        close(fd_out);
        return (EXIT_FAILURE);
    }
    
    String8 buffer = read_file(life_arena, fd_in);
    if (buffer.str)
    {
        simulate_8086(buffer, fd_out);
        fprintf(stdout, "Output written to: \"%s\"\n", filename_out);
        arena_destroy(life_arena);
        close(fd_in);
        close(fd_out);
        return (EXIT_SUCCESS);
    }

    close(fd_in);
    close(fd_out);
    arena_destroy(life_arena);
    return (EXIT_FAILURE);
}
