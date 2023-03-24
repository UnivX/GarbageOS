;boot.asm
%define STACK_SIZE 2048
%define STACK_BOTTOM_OFFSET 0x0600
%define FIRST_PARTITION_ENTRY_OFFSET 446
%define MAGIC_NUMBER_OFFSET 510
%define BASE_ADDRESS 0x7c00
;the 


org 0x7c00

bits 16

first_stage:
    jmp 0000:force_jmp;force the address to be 0000:7c00
force_jmp:
	;no interrupts and clear direction flag
	cli
    cld
	;set up segmentes and stack
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov ax, STACK_BOTTOM_OFFSET  ;the stack is located after the second part in RAM
    mov bp, ax
    add ax, STACK_SIZE
    mov sp, ax

    ;get drive number
    mov [ds:drive_number], dl

	;alive print
    mov si, msg
    call printc

	mov si, FIRST_PARTITION_ENTRY_OFFSET + BASE_ADDRESS
	
	;iterate all partitions
find_entry:
	
	cmp byte [ds:si], 0x80
	je load_entry
	
	add si, 16;next partition entry
	cmp si, MAGIC_NUMBER_OFFSET
	jne find_entry

	;if it cant find a valid partition then call it a day
	mov ax, 1
	jmp error

load_entry:
	;copy the fist lba aka VBR
	mov ax, [ds:si + 0x8]
	mov [ds:dap_lower_lba], ax
	mov ax, [ds:si + 0x8 + 0x2]
	mov [ds:dap_lower_lba + 0x2], ax
	;segment = 0

	;call the BIOS int 13h extension
	mov si, DAP
	mov ah, 0x42
	mov dl, [ds:drive_number]
	int 0x13;jc set if error
	jc error

	mov si, vbr_ok_msg
	call printc

	jmp hang


error:;jump to it when there is an error
    call print_ax
    mov si, err_msg
    call printc
    ;jmp hang

hang:;stop the cpu 
    cli
    hlt

data:
    msg db "Loading the first bootable partition: ", 0
	vbr_ok_msg db "Done", 0
    err_msg db "BAD - ", 0
    drive_number db 0
DAP:
	dap_size db 16
	dap_reserved db 0
	dap_to_read dw 1
	dap_transfer_buffer_offset dw BASE_ADDRESS+512
	dap_transfer_buffer_segment dw 0
	dap_lower_lba dd 0
	dap_upper_lba dd 0

libs:
	%include "print.asm"

    ;fill the remaining bootloader code space with zeros
    times 440-($-$$) db 0

;second sector size of 20 LBA
