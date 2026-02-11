#ifndef SIMULATE_8086_H
#define SIMULATE_8086_H

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MASK_WIDE 0
#define MASK_LOW 1
#define MASK_HIGH 2

#define POS_CF 0  // carry status flag ; 'POS' stands for position in its register
#define POS_PF 2  // parity status flag
#define POS_AF 4  // auxiliary status flag
#define POS_ZF 6  // zero status flag
#define POS_SF 7  // sign status flag
#define POS_TF 8  // overflow status flag
#define POS_IF 9  // interrupt-enable control flag
#define POS_DF 10 // direction control flag
#define POS_OF 11 // trap control flag

void simulate_8086(String8 data, s32 fd);

#endif