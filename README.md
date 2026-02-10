# 8086 Simulator
An 8086 simulator that stores and updates all states of its registers. This program processes an `.asm` input file and annotates every line with the state change of the destination register and the state change of flag register. It also outputs the final state of all registers.

## Context
This project is a solution for one of the homework assignments in Casey Muratori's [Performance-Aware Programming](https://www.computerenhance.com/) course.<br>
All example files provided under the `examples/` folder are sourced from the [computer_enhance github repo](https://github.com/cmuratori/computer_enhance).

## Usage

### Compilation
Use the `Makefile` to build the executable:
```sh
make all
```
### Run
Provide the path to an 8086 asm file as an argument:
```sh
bin/simulate8086 <filename>
```
The output of the simulation will be written to `out.txt` file in the current directory.
### Example:
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0043_immediate_movs.asm
```
*(Example output)*<br>
```asm
mov ax, 1      ; ax: 0x0000->0x0001 [         ]->[         ]
mov bx, 2      ; bx: 0x0000->0x0002 [         ]->[         ]
mov cx, 3      ; cx: 0x0000->0x0003 [         ]->[         ]
mov dx, 4      ; dx: 0x0000->0x0004 [         ]->[         ]
mov sp, 5      ; sp: 0x0000->0x0005 [         ]->[         ]
mov bp, 6      ; bp: 0x0000->0x0006 [         ]->[         ]
mov si, 7      ; si: 0x0000->0x0007 [         ]->[         ]
mov di, 8      ; di: 0x0000->0x0008 [         ]->[         ]

Final Registers:
     ax: 0x0001 (1)
     bx: 0x0002 (2)
     cx: 0x0003 (3)
     dx: 0x0004 (4)
     sp: 0x0005 (5)
     bp: 0x0006 (6)
     si: 0x0007 (7)
     di: 0x0008 (8)
     es: 0x0000 (0)
     cs: 0x0000 (0)
     ss: 0x0000 (0)
     ds: 0x0000 (0)
     [         ]
```
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0046_add_sub_cmp.asm
```
*(Example output)*<br>
```asm
mov bx, -4093  ; bx: 0x0000->0xfe67 [         ]->[         ]
mov cx, 3841   ; cx: 0x0000->0x0f01 [         ]->[         ]
sub bx, cx     ; bx: 0xfe67->0xef66 [         ]->[    S  P ]
mov sp, 998    ; sp: 0x0000->0x03e6 [    S  P ]->[    S  P ]
mov bp, 999    ; bp: 0x0000->0x03e7 [    S  P ]->[    S  P ]
cmp bp, sp     ; bp: 0x03e7->0x03e7 [    S  P ]->[         ]
add bp, 1027   ; bp: 0x03e7->0x07ea [         ]->[         ]
sub bp, 2026   ; bp: 0x07ea->0x0000 [         ]->[     Z P ]

Final Registers:
     ax: 0x0000 (0)
     bx: 0xef66 (61286)
     cx: 0x0f01 (3841)
     dx: 0x0000 (0)
     sp: 0x03e6 (998)
     bp: 0x0000 (0)
     si: 0x0000 (0)
     di: 0x0000 (0)
     es: 0x0000 (0)
     cs: 0x0000 (0)
     ss: 0x0000 (0)
     ds: 0x0000 (0)
     [     Z P ]
```
