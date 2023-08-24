load_kernel_image:
	pusha

	;----FIND /SYS DIR---
	mov esi, sys_root_name
	mov eax, [ds:fat32_root_cluster]
	call search_file_in_dir
	cmp eax, 0
	jz .kernel_not_found

	;---FIND /SYS/KRNL.BIN FILE---
	mov esi, kernel_file_name
	call search_file_in_dir
	cmp eax, 0
	jz .kernel_not_found
	mov [ds:kernel_start_cluster], eax
	mov [ds:kernel_size], edx

	;---FIND MEMORY AFTER 1MB---
	mov edx, [ds:kernel_size]
	add edx, 0x00f00000;15MB of extra space
	call find_allocatable_memory
	cmp eax, 0
	jnz .kernel_memory_error
	mov [ds:memory_map_item_for_kernel_offset], si

	;---SET THE ADDRESS WHERE TO LOAD THE KERNEL---
	mov si, [ds:memory_map_item_for_kernel_offset]
	mov eax, [ds:si] ;address 32bit
	mov [ds:kernel_image_address32], eax

	;print the addres where is loaded
	mov eax, [ds:kernel_image_address32]
	call print_eax
	mov si, newline
	call printc

	;print the cluster where is loaded
	mov eax, [ds:kernel_start_cluster]
	call print_eax
	mov si, newline
	call printc

	;LOAD FILE IN MEMORY no return value to controll
	mov edi, [ds:kernel_image_address32]
	mov eax, [ds:kernel_start_cluster]
	call load_file_in_memory
	;TODO check that the file is correctly loaded in RAM

	;calc the first frame(physical page) after the kernel
	mov eax, [ds:kernel_image_address32]
	add eax, [ds:kernel_size]
	inc eax
	call ceil_eax_4k;round up to 4k
	mov [ds:first_frame_after_kernel_image], eax

	;clean return
	mov eax, 0
.end:
	mov [ds:kernel_temp_ret], eax
	popa
	mov eax, [ds:kernel_temp_ret]
	ret

.kernel_not_found:
	mov si, kernel_not_found_msg
	call printc
	jmp hang

.kernel_memory_error:
	mov si, kernel_memory_error_msg
	call printc
	jmp hang

;out -> ds:si list item with the right memory
;out -> eax != 0 if error
;in -> edx minimum required memory
;NOTE-> this function guarantees that the returned memory is not the < 1MB memory
;and that it isn't the usable memory between the first 1MB and the ISA MEMORY HOLE(15-16MB)
;it dows not guarantee that the start address of the usable memory is after 16MB
find_allocatable_memory:
	push ebx
	push ecx
	push edx

	mov si, [ds:memory_map_offset]
	mov cx, [ds:si];cx= size of list
	add si, 8;set the si to the first item
.iterate_item:
	;if type is not 1(usable) go to the next item
	cmp dword [ds:si+16], 1
	jne .next_item


	;if the bit 0/1 of the Extended Attributes are setted incorrectly
	;if so it is not usable
	mov eax, [ds:si+20]
	and eax, 3;b11
	cmp eax, 1
	jnz .next_item

	;if the base address is not containable in a 32bit pointer then skip it
	cmp dword [ds:si+4], 0
	jnz .next_item

	;if the memory is the <1MB mem then skip
	;(it is guaranteed to be a memory hole of not usabel ram at the end of the 1MB section)
	cmp dword [ds:si], 0x000FFFFF
	jbe .next_item

	xor eax, eax;set eax to zero to be prepared to do a clean exit
	;check if the size is bigger than 2^31 -1, if so we found a suitable item
	cmp dword [ds:si+8+4], 0
	jne .fam_end

	;check if this is the 14 MB usable memory between the BIOS and the ISA memory hole
	cmp dword [ds:si+8], 0x00E00000;14MB size
	jne .no_14MB
	cmp dword [ds:si], 0x00100000;1MB start
	jne .no_14MB
	;if we are here then its the 14 MB usable memory section(it's too small and it's the only part usable by ISA)
	;so we skip it
	jmp .next_item

.no_14MB:
	;check the low part of the size
	;if the size is bigger than the requested the we found a suitable item
	cmp [ds:si+8], edx
	ja .fam_end

.next_item:
	add si, 24;go to the next item
	loop .iterate_item

	;suitable ram not found
	mov eax, 1
.fam_end:
	pop edx
	pop ecx
	pop ebx
	ret

ceil_eax_4k:
	push bx
	push eax
	mov bx, sp
	and eax, 4096-1
	jz .ceil_done
	sub dword [ss:bx], eax
	add dword [ss:bx], 4096
.ceil_done:
	pop eax
	pop bx
	ret

kernel_load_data:
	kernel_temp_ret dd 0
	kernel_file_name db "KRNL    BIN", 13, 10, 0
	kernel_not_found_msg db "cannot find /sys/krnl.bin", 13, 10, 0
	kernel_memory_error_msg db "cannot find suitable Extended RAM Memory(>1MB)", 13, 10, 0
	kernel_start_cluster dd 0
	kernel_size dd 0
	memory_map_item_for_kernel_offset dd 0
	kernel_image_address32 dd 0
	first_frame_after_kernel_image dd 0
