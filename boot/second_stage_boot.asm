%define STACK_SIZE 2048
%define STACK_BOTTOM_OFFSET 0x0600

%define TSS_SIZE_PROTECTED_MODE 0x6c
%define TSS_ADDRESS 0x0500
%define TSS_FLAGS 0x0
%define TSS_ACCESS_BYTE 0x89

org 0x7c00 + 90

start:
	cli
	;set up segmentes and stack
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov ax, STACK_BOTTOM_OFFSET  ;the stack is located after the second part in RAM
    mov bp, ax
    add ax, STACK_SIZE
    mov sp, ax
	mov ax, 2

	;alive print
    mov si, msg
    call printc

	;disable_nmi
	in al, 0x70
	or al, 0x80
	out 0x80, al
	in al, 0x71

	;check if a20 bios functions are supported
	mov ax, 2403h
	int 15h
	jb a20_error
	cmp ah, 0
	jnz a20_error

	;check if a20 is enabled via bios
	mov ax, 2402h
	int 15h
	jb a20_error
	cmp ah, 0
	jnz a20_error

	cmp al, 1
	jz a20_enabled

	;enable a20 via bios
	mov ax, 2403h
	int 15h
	jb a20_error
	cmp ah, 0
	jnz a20_error

a20_enabled:
	mov si, a20_good
	call printc

	;setup GDT

	;generate TSS
	mov word [ds:GDT_TSS], TSS_SIZE_PROTECTED_MODE
	mov word [ds:GDT_TSS+16], TSS_ADDRESS
	mov byte [ds:GDT_TSS+40], TSS_ACCESS_BYTE
	or byte [ds:GDT_TSS+52], TSS_FLAGS

	;load gdt
	mov ax, GDT
	mov [ds:gdtr+2], ax
	mov ax, GDT-GDT_END
	mov [ds:gdtr], ax
	lgdt [ds:gdtr]


	;reload segment registers
	;TODO

	jmp hang
a20_error:
	mov si, a20_err
	call printc

hang:;stop the cpu 
    cli
    hlt

data:
	a20_err db "cannot enable a20 gate", 13, 10, 0
	a20_good db "a20 gate enabled", 13, 10, 0
    msg db "second stage loaded correclty", 13, 10, 0

GDT:
	dq 0x0000000000000000;NULL
	dq 0x00CF9A000000FFFF;Kernel mode code segment
	dq 0x00CF92000000FFFF;Kernel mode data segment
	dq 0x00CFFA000000FFFF;User mode code segment
	dq 0x00CFF2000000FFFF;User mode data segment
GDT_TSS:
	dq 0x0000000000000000;TSS todo
GDT_END:

gdtr:
	dw 0;limit storage
	dd 0;base storage

libs:
	%include "print.asm"

times 420-($-$$) db 0
