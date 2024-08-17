%define STACK_SIZE 2048
%define VBE_BUFFER_SIZE 1024
%define STACK_BOTTOM_OFFSET 0x0610
%define FREE_LOW_MEM_ADDR STACK_SIZE+STACK_BOTTOM_OFFSET
%define BK_BOOT_SEC_ADDRESS 0x7c00 + 50

org 0x7c00 + 90
bits 16
;MEMORY MAP:
;0x00000000 - 0x000004FF FREE but usable only in long mode or protected_mode
;0x00000500 - 0x000005FF FREE
;0x00000600 - 0x000007FF STACK
;0x00000800 - 0x00007BFF FREE
;0x00001000 - 0x00001007 BootLoaderData pointer
;0x00001008 - 0x00001FFF FREE
;0x00002000 - 0x00003FFF RESERVED for OS APs startup
;0x00004000 - 0x00007BFF FREE
;0x00007C00 - 0x00007DFF VBR (second_stage)
;0x00007E00 - 0x0000FFFF third stage + other run time data
;0x00010000 - 0x0007FFFF FREE

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

	;LOADING FROM PRINT
    mov si, loading_from_msg
    call printc
	mov ax, [ds:BK_BOOT_SEC_ADDRESS]
	add ax, 3;size of backup
	add ax, [ds:partition_offset]
	call print_ax
    mov si, newline
    call printc

	;LOADING THE THIRD STAGE
	;set the lba
	xor eax, eax
	mov ax, [ds:BK_BOOT_SEC_ADDRESS]
	add eax, 3;size of backup 
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

error:;jump to it when there is an error
    call print_ax
    mov si, err_msg
    call printc
    ;jmp hang

hang:;stop the cpu 
	mov si, hang_msg
	call printc
    cli
.rhang:
	hlt
	jmp .rhang

data:
	err_msg db " error, ax=", 13, 10, 0
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
	dap_to_read dw 20
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

	;----CHECK THE COMPATIBILITY----
	call check_cpuid
	call check_long_mode

	;----PROTECTED MODE SETUP----
	call enable_a20
	call disable_nmi

	push ds
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
;it doesn't work with bits 32?????
;bits 32
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
	pop ds

	call enable_nmi

	mov si, unreal_mode_good
	call printc

	;---GET MEMORY MAP---
	mov di, [ds:free_memory_offset]
	mov [ds:memory_map_offset], di
	call get_memory_map
	mov [ds:free_memory_offset], di
	mov si, memory_detection_good
	call printc

	;---INIT FAT32 DRV---
	mov dl, [ds:drive_number]
	xor eax, eax
	mov ax, FREE_LOW_MEM_ADDR
	call init_fat32_driver
	mov si, fat32_init_msg
	call printc

	;----FIND /SYS DIR---
	mov esi, sys_root_name
	mov eax, [ds:fat32_root_cluster]
	call search_file_in_dir
	cmp eax, 0
	jz file_or_dir_not_found

	;---FIND /SYS/T.TXT FILE---
	mov esi, test_file_name
	call search_file_in_dir
	cmp eax, 0
	jz file_or_dir_not_found
	;eax-> file_start_cluster
	;edx-> file_size

	;---LOAD FILE---
	push edx
	mov edi, 0x01000000
	call load_file_in_memory
	pop edx

	;---PRINT FILE---
	mov byte [ds:edi+edx], 13
	mov byte [ds:edi+edx+1], 0
	mov esi, 0x01000000
	call printc_unreal

	;---INIT VBE DRV---
	mov di, [ds:free_memory_offset]
	call init_vbe
	add word [ds:free_memory_offset], VBE_BUFFER_SIZE;1024
	mov si, vbe_init_msg
	call printc

	;---LOAD KERNEL ELF IMAGE---
	call load_kernel_image
	mov si, kernel_load_elf_good
	call printc

	;---SET UP FRAME ALLOCATOR
	mov eax, [ds:first_frame_after_kernel_image]
	mov ebx, [ds:memory_map_item_for_kernel_offset]
	call init_frame_allocator

	;---TRY LOAD 720X480
	mov ax, 640
	mov bx, 480
	mov cx, 32
	;call try_load_mode
	;cmp ax, 0
	;je vbe_good


	;---TRY LOAD 1024X768 32 DEPTH---
	mov ax, 1024
	mov bx, 768
	mov cx, 32
	call try_load_mode
	cmp ax, 0
	je vbe_good

	;---TRY LOAD 1024X768 24 DEPTH---
	mov ax, 1024
	mov bx, 768
	mov cx, 24
	call try_load_mode
	cmp ax, 0
	je vbe_good
	jmp vbe_mode_error
vbe_good:
	mov cl, 0xff
	call fill_screen_grey_scale
	jmp set_up_long_mode

.true_loop:
	jmp .true_loop

file_or_dir_not_found:
	mov si, dir_or_file_not_found_msg
	call printc
	jmp hang

vbe_mode_error:
	mov si, vbe_mode_error_msg
	call printc
	jmp hang



third_stage_data:
	vbe_mode_error_msg db "error while setting vbe mode", 13, 10, 0
	dir_or_file_not_found_msg db "dir or file not found", 13, 10, 0
	unreal_mode_good db "unreal mode good", 13, 10, 0
	gdt_loaded db "gdt loaded", 13, 10, 0
	fat32_init_msg db "fat32 driver init good", 13, 10, 0
	vbe_init_msg db "vbe driver init good", 13, 10, 0
	test_file_name db "T       TXT", 13, 10, 0
	sys_root_name db "SYS        ", 13, 10, 0
	memory_detection_good db "memory detection good", 13, 10, 0
	kernel_load_elf_good db "kernel elf image loaded", 13, 10, 0
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
	%include "vbe.asm"
	%include "check_features.asm"
	%include "a20_enable.asm"
	%include "nmi.asm"
	%include "kernel_load.asm"
	%include "long_mode.asm"
	%include "frame_allocator.asm"
END_OF_BOOTLOADER:
