%define STACK_SIZE 2048
%define STACK_BOTTOM_OFFSET 0x0600
%define FREE_LOW_MEM_ADDR STACK_SIZE+STACK_BOTTOM_OFFSET
%define TSS_SIZE_PROTECTED_MODE 0x6c
%define TSS_ADDRESS 0x0500
%define TSS_FLAGS 0x0
%define TSS_ACCESS_BYTE 0x89
%define BK_BOOT_SEC_ADDRESS 0x7c00 + 50

org 0x7c00 + 90
bits 16

start:
	jmp 0x0000:second_stage 
second_stage:
	cli

	;get loaded partion offset
	mov [ds:partition_offset], eax
    ;get drive number
    mov [ds:drive_number], dl

	;set up segmentes and stack
    xor ax, ax
    mov ss, ax
    mov ds, ax
	mov es, ax
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
	add ax, 3;size of backup
	add ax, [ds:partition_offset]
	call print_ax

    mov si, newline
    call printc

	xor eax, eax
	mov ax, [ds:BK_BOOT_SEC_ADDRESS]
	add al, 3;size of backup 
	add eax, [ds:partition_offset]
	mov [ds:dap_lower_lba], eax
	;lower_lba+2 = 0

	;call the BIOS int 13h extension
	mov si, DAP
	mov ah, 0x42
	mov dl, [ds:drive_number]
	int 0x13;jc set if error
	jc error

	;now that the third stage is loaded jmp
	jmp 0x0000:third_stage_entry


no_64bit_error:
	mov si, no_64bit_err
	call printc
	jmp hang

error:;jump to it when there is an error
    call print_ax
    mov si, err_msg
    call printc
    ;jmp hang

hang:;stop the cpu 
	mov si, hang_msg
	call printc
    cli
    hlt

data:
	err_msg db " error, ax=", 13, 10, 0
	no_64bit_err db "cant boot up, the system isn't x86_64", 13, 10, 0
    msg db "stage 2 good", 13, 10, 0
	loading_from_msg db "loading from: ", 0
	hang_msg db "hanging...", 13, 10, 0
	newline db 13, 10, 0
    drive_number db 0
	partition_offset dd 0
	align 2
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
	;notify bios of the target mode (long mode)
	mov ax, 0xec00
	mov bl, 2
	int 15h


	;----PROTECTION MODE SETUP----
	;disable_nmi
	in al, 0x70
	or al, 0x80
	out 0x70, al
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

	;load gdt
	lgdt [unreal_gdtr]

	mov si, gdt_loaded
	call printc

	;set PE(Protection Enable)
	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 08h:protected_mode

protected_mode:
bits 32
	;now we will load the segment descriptor for the kernel data
	;the CPU will load in the segment cache the options, after we return to real mode
	;the cache will remain unaffected and we will be able to write
	;over the 1MB barrier(the limit are that of the flat cached segment descriptor)

	;select kernel mode data segment descriptor
	mov bx, 0x10
	mov ds, bx

	;back to real mode(unreal_mode)
	;eax still contains a copy of cr0

	and al, 0xFE ;clear PE(Protection Enable)
	mov cr0, eax
	jmp 0x0:unreal_mode

unreal_mode:
bits 16
	xor ax, ax
	mov ds, ax

	mov si, unreal_mode_good
	call printc


	mov dl, [ds:drive_number]
	xor eax, eax
	mov ax, FREE_LOW_MEM_ADDR
	call init_fat32_driver

	mov si, fat32_init_msg
	call printc

	mov esi, sys_root_name
	mov eax, [ds:fat32_root_cluster]
	call search_file_in_dir
	cmp eax, 0
	jz file_or_dir_not_found

	mov esi, test_file_name
	call search_file_in_dir
	cmp eax, 0
	jz file_or_dir_not_found

	;eax-> file_start_cluster
	;edx-> file_size
	push edx
	mov edi, 0x01000000
	call load_file_in_memory
	pop edx

	mov byte [ds:edi+edx], 13
	mov byte [ds:edi+edx+1], 0
	mov esi, 0x01000000
	call printc_unreal

	;enable_nmi
	in al, 0x70
	and al, 0x7F
	out 0x70, al
	in al, 0x71

	
	mov di, [ds:free_memory_offset]
	mov [ds:memory_map_offset], di
	call get_memory_map
	mov [ds:free_memory_offset], di

	mov si, memory_detection_good
	call printc


	jmp hang

file_or_dir_not_found:
	mov si, dir_or_file_not_found_msg
	call printc
	jmp hang

a20_error:
	mov si, a20_err
	call printc
	jmp hang


third_stage_data:
	dir_or_file_not_found_msg db "dir or file not found", 13, 10, 0
	a20_err db "a20 err", 13, 10, 0
	a20_good db "a20 good", 13, 10, 0
	unreal_mode_good db "unreal mode good", 13, 10, 0
	gdt_loaded db "gdt loaded", 13, 10, 0
	fat32_init_msg db "fat32 driver init good", 13, 10, 0
	test_file_name db "T       TXT", 13, 10, 0
	sys_root_name db "SYS        ", 13, 10, 0
	memory_detection_good db "memory detection good", 13, 10, 0
	free_memory_offset dw END_OF_BOOTLOADER
	memory_map_offset dd 0

UNREAL_GDT:
	dq 0x0000000000000000;NULL
	db 0xff, 0xff, 0, 0, 0, 10011010b, 00000000b, 0;code segment, 16bit mode
	db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0;data segment, 32bit mode
UNREAL_GDT_END:

unreal_gdtr:
	dw UNREAL_GDT_END-UNREAL_GDT-1;limit storage
	dd UNREAL_GDT;base storage

third_stage_libs:

	%include "fat32.asm"
	%include "memory_detect.asm"
END_OF_BOOTLOADER:
