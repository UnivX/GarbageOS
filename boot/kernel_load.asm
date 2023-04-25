load_kernel_image:
	pusha
	mov bx, [ds:memory_map_offset]

.end:
	mov [ds:kernel_temp_ret], eax
	popa
	mov eax, [ds:kernel_temp_ret]
	ret


kernel_load_data:
	kernel_temp_ret dd 0
