;boot.asm
%define SECOND_PART_SIZE 20
%define REAL_SECOND_PART_SIZE (SECOND_PART_SIZE * 512)
;the 


org 0x7c00

bits 16

first_stage:
    jmp 0000:force_jmp;force the address to be 0000:7c00
force_jmp:
    ;alloc 1024 bytes of stack after the bootloader
    cld;clear direction flag
	cli;no interrupts
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov ax, stack_bottom  ;the stack is located after the second part in RAM
    mov bp, ax
    add ax, 1024
    mov sp, ax

    mov si, msg
    call printc

    ;get drive number
    mov [ds:drive_number], dl

error:;jump to it when there is an error
    call print_ax
    mov si, err_msg
    call printc
    ;jmp hang

hang:;stop the cpu 
    cli
    hlt

data:
    msg db "LD 1' PRT - ", 0
    err_msg db "BAD - ", 0
    ok_msg db "OK - ", 0
    drive_param_ptr dw 0
    allocated_mem_offset dw 0
    sector_size dw 0
    drive_number db 0
libs:
	%include "print.asm"

    ;fill the remaining bootloader code space with zeros
    times 440-($-$$) db 0

;second sector size of 20 LBA
s_entry:
    mov si, second_stage_msg
    call printc
	jmp hang
s_data:
	second_stage_msg db "Loaded second part - ", 0

times 512 + REAL_SECOND_PART_SIZE - ($-$$) db 0

stack_bottom:
