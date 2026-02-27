# 8086 Simulator
The program can run in 2 different modes.<br>
**The first mode** is disassembly mode - program will convert binary machine code into readable assembly.<br>
**The second mode** is simulation/execution mode - program will simulate 8086 CPU. Such that is stores and updates all states of the cpu's registers. It processes a binary machine code file, converts into readable assembly and annotates every line with the state change of the destination register and the state change of flag and ip registers. It also outputs the final state of all registers.

## Context
This project is a solution for one of the homework assignments in Casey Muratori's [Performance-Aware Programming](https://www.computerenhance.com/) course.<br>
All example files provided under the `examples/` folder are sourced from the [computer_enhance github repo](https://github.com/cmuratori/computer_enhance).

## References
[Intel 8086 Family User's Manual October 1979](https://archive.org/details/bitsavers_intel80869lyUsersManualOct79_62967963/page/n1/mode/2up) [archive.org]<br>

## Usage

### Compilation
Use the `Makefile` to build the executable:
```sh
make all
```
### Run
Provide the path to an 8086 asm file as an argument and provide optional ```-exec``` flag:
```sh
bin/simulate8086 <filename> -exec
```
If no flag is provided, such as:
```sh
bin/simulate8086 <filename>
```
The program will output only the disassembly of the input file.<br>
The output of the simulation will be written to `out.txt` file in the current directory.
### Examples:
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0038_many_register_mov
```
*(Example output)*<br>
```asm
bits 16

mov cx, bx   
mov ch, ah   
mov dx, bx   
mov si, bx   
mov bx, di   
mov al, cl   
mov ch, ch   
mov bx, ax   
mov bx, si   
mov sp, di   
mov bp, ax   
```
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0043_immediate_movs.asm -exec
```
*(Example output)*<br>
```asm
bits 16

mov ax, 1     ; ax: 0x0000->0x0001 ip: 0x0000->0x0003 
mov bx, 2     ; bx: 0x0000->0x0002 ip: 0x0003->0x0006 
mov cx, 3     ; cx: 0x0000->0x0003 ip: 0x0006->0x0009 
mov dx, 4     ; dx: 0x0000->0x0004 ip: 0x0009->0x000c 
mov sp, 5     ; sp: 0x0000->0x0005 ip: 0x000c->0x000f 
mov bp, 6     ; bp: 0x0000->0x0006 ip: 0x000f->0x0012 
mov si, 7     ; si: 0x0000->0x0007 ip: 0x0012->0x0015 
mov di, 8     ; di: 0x0000->0x0008 ip: 0x0015->0x0018 

Final Registers:
        ax: 0x0001 (1)
        bx: 0x0002 (2)
        cx: 0x0003 (3)
        dx: 0x0004 (4)
        sp: 0x0005 (5)
        bp: 0x0006 (6)
        si: 0x0007 (7)
        di: 0x0008 (8)
        ip: 0x0018 (24)
     flags: [         ]
```
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0046_add_sub_cmp.asm -exec
```
*(Example output)*<br>
```asm
bits 16

mov bx, 61443 ; bx: 0x0000->0xf003 ip: 0x0000->0x0003 
mov cx, 3841  ; cx: 0x0000->0x0f01 ip: 0x0003->0x0006 
sub bx, cx    ; bx: 0xf003->0xe102 ip: 0x0006->0x0008 [         ]->[    S    ]
mov sp, 998   ; sp: 0x0000->0x03e6 ip: 0x0008->0x000b 
mov bp, 999   ; bp: 0x0000->0x03e7 ip: 0x000b->0x000e 
cmp bp, sp    ; bp: 0x03e7->0x03e7 ip: 0x000e->0x0010 [    S    ]->[         ]
add bp, 1027  ; bp: 0x03e7->0x07ea ip: 0x0010->0x0014 [         ]->[         ]
sub bp, 2026  ; bp: 0x07ea->0x0000 ip: 0x0014->0x0018 [         ]->[     Z P ]

Final Registers:
        bx: 0xe102 (57602)
        cx: 0x0f01 (3841)
        sp: 0x03e6 (998)
        ip: 0x0018 (24)
     flags: [     Z P ]
```
*(Example input)*<br>
```sh
bin/simulate8086 examples/listing_0049_conditional_jumps -exec
```
*(Example output)*<br>
```asm
bits 16

mov cx, 3     ; cx: 0x0000->0x0003 ip: 0x0000->0x0003 
mov bx, 1000  ; bx: 0x0000->0x03e8 ip: 0x0003->0x0006 
add bx, 10    ; bx: 0x03e8->0x03f2 ip: 0x0006->0x0009 [         ]->[         ]
sub cx, 1     ; cx: 0x0003->0x0002 ip: 0x0009->0x000c [         ]->[         ]
jnz 6         ;                    ip: 0x000c->0x0006 
add bx, 10    ; bx: 0x03f2->0x03fc ip: 0x0006->0x0009 [         ]->[       P ]
sub cx, 1     ; cx: 0x0002->0x0001 ip: 0x0009->0x000c [       P ]->[         ]
jnz 6         ;                    ip: 0x000c->0x0006 
add bx, 10    ; bx: 0x03fc->0x0406 ip: 0x0006->0x0009 [         ]->[       P ]
sub cx, 1     ; cx: 0x0001->0x0000 ip: 0x0009->0x000c [       P ]->[     Z P ]
jnz 6         ;                    ip: 0x000c->0x000e 

Final Registers:
        bx: 0x0406 (1030)
        ip: 0x000e (14)
     flags: [     Z P ]
```
