#include "common.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    (void)argv;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", PROGRAM_PATH);
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}
