;temporary frame allocator
;in -> eax first_frame_address
;in -> ebx addresss to the item in the memory map that rappresent the memory area
init_frame_allocator:
	mov [ds:first_frame_address], eax
	mov [ds:memory_map_item_offset], ebx
	ret

;use this function only after the correct load of a linear bufer VBE mode
;out -> eax 32 bit address of the frame
alloc_frame:
	push ebx
	mov bx, [ds:memory_map_item_offset]
	;if the lenght of the region is above 2^32-1 then we can allocate the frame
	;pretty sure that we wont use all of the 2^32-1 bytes in the boot stage
	mov eax, [ds:bx+4+8]
	test eax, eax
	jnz .there_is_frame
	;check if we are out of memory
	mov eax, [ds:first_frame_address]
	sub eax, [ds:bx];calculate the difference between the addresses
	cmp eax, [ds:bx+8];compare the difference with the size of the area
	jae .alloc_frame_error
.there_is_frame:
	;alloc the frame
	mov eax, [ds:first_frame_address]
	add dword [ds:first_frame_address], 4096

	pop ebx
	ret

.alloc_frame_error:
	mov cl, 128
	call fill_screen_grey_scale
.endless_loop:
	hlt
	jmp .endless_loop

	

memory_map_item_offset dd 0
first_frame_address dd 0
