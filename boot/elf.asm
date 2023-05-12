bits 64
load_elf:
	xor rax, rax
	mov ebx, [kernel_image_address32]
	mov [kernel_image_address64], rbx
	mov eax, [rbx]
	cmp eax, 0x464c457f;elf magic number
	jne .elf_load_err
	ret

.elf_load_err:
	mov rdx, 0xBAAD0E1F
	hlt
	jmp .elf_load_err

kernel_image_address64 dq 0
