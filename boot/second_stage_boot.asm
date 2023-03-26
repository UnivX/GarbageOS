%define STACK_SIZE 2048
%define STACK_BOTTOM_OFFSET 0x0600

%define TSS_SIZE_PROTECTED_MODE 0x6c
%define TSS_ADDRESS 0x0500
%define TSS_FLAGS 0x0
%define TSS_ACCESS_BYTE 0x89
%define BK_BOOT_SEC_ADDRESS 0x7c00 + 50

org 0x7c00 + 90

start:
	jmp 0x0000:second_stage
second_stage:
	cli

    ;get drive number
    mov [ds:drive_number], dl

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
	
	;check if the cpu is 64 bit
	mov eax, 0x80000001
	cpuid
	test edx, (1 << 29)
	jz no_64bit_error


	;load the third stage
	;set the lba

    mov si, loading_from_msg
    call printc

	mov ax, [ds:BK_BOOT_SEC_ADDRESS]
	add ax, 4
	call print_ax

    mov si, newline
    call printc

	mov ax, [ds:BK_BOOT_SEC_ADDRESS]
	add al, 4
	mov [ds:dap_lower_lba], ax
	;lower_lba+2 = 0

	;call the BIOS int 13h extension
	mov si, DAP
	mov ah, 0x42
	mov dl, [ds:drive_number]
	int 0x13;jc set if error
	jc error

    mov si, going_third_msg
    call printc

	mov ax, [ds:third_stage_entry]

	;now that the third stage is loaded jmp
	jmp 0x0000:third_stage_entry

error:;jump to it when there is an error
    call print_ax
    mov si, err_msg
    call printc
    ;jmp hang

hang:;stop the cpu 
    cli
    hlt

data:
	err_msg db " error, ax=", 13, 10, 0
	no_64bit_err db "long mode err", 13, 10, 0
    msg db "stg 2 good", 13, 10, 0
    going_third_msg db "jmp to third", 13, 10, 0
	loading_from_msg db "loading from: ", 0
	newline db 13, 10, 0
    drive_number db 0
DAP:
	dap_size db 16
	dap_reserved db 0
	dap_to_read dw 8
	dap_transfer_buffer_offset dw third_stage_entry
	dap_transfer_buffer_segment dw 0
	dap_lower_lba dd 0
	dap_upper_lba dd 0

libs:
	%include "print.asm"

times 420-($-$$) db 0





third_stage_entry:
	nop
	;----PROTECTION MODE SETUP----
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


	;set PE(Protection Enable)
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 08h:protected_mode

protected_mode:
	jmp hang

no_64bit_error:
	mov si, no_64bit_err
	call printc
	jmp hang

a20_error:
	mov si, a20_err
	call printc
	jmp hang


third_stage_data:
	a20_err db "a20 err", 13, 10, 0
	a20_good db "a20 good", 13, 10, 0

GDT:
	dq 0x0000000000000000;NULL
	dq 0x00CF9A000000FFFF;Kernel mode code segment
	dq 0x00CF92000000FFFF;Kernel mode data segment
	dq 0x00CFFA000000FFFF;User mode code segment
	dq 0x00CFF2000000FFFF;User mode data segment
GDT_TSS:
	dq 0x0000000000000000;TSS setup done in code
GDT_END:

gdtr:
	dw 0;limit storage
	dd 0;base storage
