%define PT_LOAD 0x00000001

bits 64
load_elf:
	xor rax, rax
	xor rbx, rbx
	mov ebx, [kernel_image_address32]
	mov [kernel_image_address64], rbx


	;check signature
	mov eax, [rbx]
	cmp eax, 0x464c457f;elf magic number
	jne elf_load_err

	;check 64bit
	mov al, [rbx+4]
	cmp al, 2
	jne elf_load_err

	;check elf version
	mov al, [rbx+6]
	cmp al, 1
	jne elf_load_err

	;check that the ABI is System V
	mov al, [rbx+7]
	cmp al, 0
	jne elf_load_err

	mov al, [rbx+0x14]
	cmp al, 1
	jne elf_load_err

	;retrive entry point
	mov rax, [rbx+0x18]
	mov [kernel_entry_point], rax

	;retrive program header data
	mov rax, [rbx+0x20]
	mov [program_header_offset], rax

	mov ax, [rbx+0x36]
	mov [program_header_entry_size], ax

	mov ax, [rbx+0x38]
	mov [program_header_entries_number], ax

	call parse_program_header

	ret

parse_program_header:
	push rbx
	push rcx
	push rax
	push rdx

	mov rbx, [kernel_image_address64]
	add rbx, [program_header_offset]
	;rbx = program_header_address entry in RAM

	xor rcx, rcx
	mov cx, [program_header_entries_number]
.loop_table:
	;check if it's a loadable segment
	mov eax, [rbx+0x00]
	cmp eax, PT_LOAD
	jne .next_entry

	;if it's a loadable segment
	call load_segment

.next_entry:
	xor rdx, rdx
	mov dx, [program_header_entry_size]
	add rbx, rdx
	loop .loop_table


	pop rdx
	pop rax
	pop rcx
	pop rbx
	ret

;in bx -> program_header entry address RAM
load_segment:
	push rbx
	push rcx
	push rax
	push rdx
	push rdi
	push rsi

	;get mem size
	;with roundup to 4k
	mov rcx, [rbx+0x28]
	test cx, 0x0fff
	jz .no_add4k
	add rcx, 4096
.no_add4k:
	shr rcx, 12;divide by 4k

	;alloc_mem
	;rcx = number of pages to alloc 
	;rdi = dest vaddr
	mov rdi, [rbx+0x10]
.alloc_mem:
	push rcx
	push rbx;save rbx

	;get the flags
	xor rcx, rcx
	mov cl, [rbx+0x4]
	and cl, 2
	mov rbx, rdi
	call lmode_alloc_frame;rax = frame
	;rcx = flags, rbx= virtual addr, rax=frame
	;NOTE: even if the page doesn't have write permission, ring0 processes can write to it since the WP bit in cr0 is set (by default)
	call mmap

	pop rbx
	pop rcx
	add rdi, 4096
	loop .alloc_mem

	;copy the file data
	;rsi=elf file addr
	mov rsi, [kernel_image_address64]
	add rsi, [rbx+0x08]
	;rdi=destination
	mov rdi, [rbx+0x10]
	;rcx=file size
	mov rcx, [rbx+0x20]
	call mem_cpy

	;zero out the rest of the mem
	add rdi, rcx
	mov rax, rcx
	mov rcx, [rbx+0x28]
	sub rcx, rax
	;rdi=destination + file_size
	;rcx = mem_size-file_size
	call zero_mem

	pop rsi
	pop rdi
	pop rdx
	pop rax
	pop rcx
	pop rbx
	ret

elf_load_err:
	mov rdx, 0xBAAD0E1F
	jmp elf_load_err

;rdi = destination
;rsi = source
;rcx = size
mem_cpy:
	push rax
	push rsi
	push rdi
	push rcx
.copy8:
	cmp rcx, 8
	jb .copy1
	mov rax, [rsi]
	mov [rdi], rax

	add rsi, 8
	add rdi, 8
	sub rcx, 8
	jmp .copy8

.copy1:
	cmp rcx, 0
	jz .end
	mov al, [rsi]
	mov [rdi], al

	inc rsi
	inc rdi
	dec ecx
	jmp .copy1
.end:
	pop rcx
	pop rdi
	pop rsi
	pop rax
	ret


;rdi = destination
;rcx = size
zero_mem:
	push rdi
	push rcx
.copy8:
	cmp rcx, 8
	jb .copy1
	mov qword [rdi], 0

	add rsi, 8
	add rdi, 8
	sub rcx, 8
	jmp .copy8

.copy1:
	cmp rcx, 0
	jz .end
	mov byte [rdi], 0

	inc rsi
	inc rdi
	dec ecx
	jmp .copy1
.end:
	pop rcx
	pop rdi
	ret

call_kernel:
	mov rax, [kernel_entry_point]
	xor rbx, rbx
	mov bx, [vbe_mode_info_struct]
	mov [0x0600], rbx
	mov rbx, frame_allocator_data
	mov [0x0608], rbx
	jmp rax

kernel_image_address64 dq 0
kernel_entry_point dq 0
program_header_offset dq 0
program_header_entry_size dw 0
program_header_entries_number dw 0
