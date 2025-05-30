; 8080 assembler code
.hexfile Sum.hex
.binfile Sum.com
; try "hex" for downloading in hex format
.download bin  
.objcopy gobjcopy
.postbuild echo "OK!"
;.nodump

; OS call list
PRINT_B		equ 4
PRINT_MEM	equ 3
READ_B		equ 7
READ_MEM	equ 2
PRINT_STR	equ 1
READ_STR	equ 8
PROCESS_EXIT	equ 9
SET_QUANTUM	equ 6

; Position for stack pointer
stack   equ 04000h

org 000H
jmp begin

; Start of our Operating System
GTU_OS:	PUSH D
	push D
	push H
	push psw
	nop	; This is where we run our OS in C++, see the CPU8080::isSystemCall()
		; function for the detail.
	pop psw
	pop h
	pop d
	pop D
	ret

sum	ds 2 ; will keep the sum

begin:
	LXI SP,stack 	; always initialize the stack pointer
   	mvi c, 20	; init C with 20
	mvi a, 0	; A = 0
loop:
	ADD c		; A = A + C
	DCR c		; --C
	JNZ loop	; goto loop if C!=0
	STA sum		; sum = A
	LDA sum		; A = sum
	; Now we will call the OS to print the value of sum
	MOV B, A	; B = A
	MVI A, PRINT_B	; store the OS call code to A
	call GTU_OS	; call the OS

	MVI A,PROCESS_EXIT
	CALL GTU_OS
