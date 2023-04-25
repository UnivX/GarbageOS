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
	call find_allocatable_memory
	cmp eax, 0
	jnz .kernel_memory_error
	mov [ds:memory_map_item_for_kernel_offset], si

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

	;if the bit 0/1 of the Extended Attributes is set
	;if so it is not usable
	mov eax, [ds:si+20]
	and eax, 3;b11
	jnz .next_item

	;if the base address is not containable in a 32bit pointer then skip it
	cmp dword [ds:si+4], 0
	jnz .next_item

	xor eax, eax;set eax to zero to be prepared to do a clean exit
	;check if the size is bigger than 2^31 -1, if so we found a suitable item
	cmp dword [ds:si+8+4], 0
	jne .fam_end

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

kernel_load_data:
	kernel_temp_ret dd 0
	kernel_file_name db "KRNL    BIN", 13, 10, 0
	kernel_not_found_msg db "cannot find /sys/krnl.bin", 13, 10, 0
	kernel_memory_error_msg db "cannot find suitable Extended RAM Memory(>1MB)", 13, 10, 0
	kernel_start_cluster dd 0
	kernel_size dd 0
	memory_map_item_for_kernel_offset dw 0
