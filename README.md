# 8086 Simulator
This program simulates an Intel 8086 processor and can run in three distinct modes:

* **Disassembly Mode (`-disasm`):** Converts binary machine code into readable assembly language.

* **Simulation/Execution Mode (`-exec`):** Simulates the 8086 CPU by tracking and updating all internal register states. It processes the binary machine code, outputs the readable assembly, and annotates every line with the state changes of the destination register, the flags, and the instruction pointer (IP). It also outputs the final state of all registers that hold a non-zero value.
  * *Optional:* Providing an extra `-dump` flag will dump the entire 1MB content of the simulated memory to a `sim86_memory.data` file.

* **Profiler Mode (`-showclocks` or `-explainclocks`):** Processes the binary, converts it to readable assembly, and annotates every line with the estimate clock-cycle execution time for each instruction.
  * *Optional:* Using the `-explainclocks` flag instead of `-showclocks` will add an extra formula breakdown explaining exactly how the final cycle count was calculated (e.g., base clocks + Effective Address calculation penalties).
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
or just compile directly with gcc:
```sh
gcc src/main.c -o simulate8086
```
### Run
Provide the path to an 8086 asm file as an argument and provide optional ```-exec``` flag:<br>
***Execution/Simulation Mode:***<br>
```sh
./simulate8086 <filename> -exec
```
***Disassembly Mode:***<br>
```sh
./simulate8086 <filename>
```
The output of the simulation will be written to `out.txt` file in the current directory.
### Examples:
*(Example input)*<br>
```sh
./simulate8086 examples/listing_0038_many_register_mov
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
./simulate8086 examples/listing_0043_immediate_movs.asm -exec
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
./simulate8086 examples/listing_0046_add_sub_cmp.asm -exec
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
./simulate8086 examples/listing_0049_conditional_jumps -exec
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
*(Example input)*<br>
```sh
/simulate8086 examples/listing_0057_challenge_cycles -explainclocks
```
*(Example output)*<br>
```asm
bits 16

                            ; Clock Counter (Profiler)
mov bx, 1000                ; +4  = 4   
mov bp, 2000                ; +4  = 8   
mov si, 3000                ; +4  = 12  
mov di, 4000                ; +4  = 16  
mov cx, [bp + di]           ; +15 = 31  (8 + 7ea)
mov [bx + si], cx           ; +16 = 47  (9 + 7ea)
mov cx, [bp + si]           ; +16 = 63  (8 + 8ea)
mov [bx + di], cx           ; +17 = 80  (9 + 8ea)
mov cx, [bp + di + 1000]    ; +19 = 99  (8 + 11ea)
mov [bx + si + 1000], cx    ; +20 = 119 (9 + 11ea)
mov cx, [bp + si + 1000]    ; +20 = 139 (8 + 12ea)
mov [bx + di + 1000], cx    ; +21 = 160 (9 + 12ea)
add dx, [bp + si + 1000]    ; +21 = 181 (9 + 12ea)
add [bp + si], word 76      ; +25 = 206 (17 + 8ea)
add dx, [bp + si + 1001]    ; +25 = 231 (9 + 12ea + 4p)
add [di + 999], dx          ; +33 = 264 (16 + 9ea + 8p)
add [bp + si], word 75      ; +25 = 289 (17 + 8ea)
```