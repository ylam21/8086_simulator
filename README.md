# 8086 Simulator

## Context
This project is a solution for one of the homework assignments in Casey Muratori's [Performance-Aware Programming](https://www.computerenhance.com/) course.<br>

## Usage

### Compilation
Use `Makefile` to build the executable:
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
mov ax, 1 ; ax:0->1
mov bx, 2 ; bx:0->2
mov cx, 3 ; cx:0->3
mov dx, 4 ; dx:0->4
mov sp, 5 ; sp:0->5
mov bp, 6 ; bp:0->6
mov si, 7 ; si:0->7
mov di, 8 ; di:0->8

Final Registers:
     ax: 0x0001 (1)
     bx: 0x0002 (2)
     cx: 0x0003 (3)
     dx: 0x0004 (4)
     sp: 0x0005 (5)
     bp: 0x0006 (6)
     si: 0x0007 (7)
     di: 0x0008 (8)
```
