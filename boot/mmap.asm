bits 64 
;in rax -> physical address
;in rbx -> virtual address
mmap:
	push rcx
	push rdx
	push rdi
	push rbx
	mov rdi, cr3
	and di, 0xf000
	;rdi used to contain the page map table
	;rdx used to calculate the index
	;rcx used for masking
	;48 bit paging so we do not use the first 16 bit
	;for other info search for 'canonical addresses in x86_64'
	mov rcx, 0x0000fffffffff000
	and rax, rcx

	;rdi=PML4 
	mov rdx, rbx
	shr rdx, 39
	mov rcx, 0x1ff;9bit masking
	and rdx, rcx
	call set_entry_if_not_present
	mov rdi, [rdi+rdx*8]
	mov rcx, 0x0000fffffffff000
	and rdi, rcx

	;rdi=PDPT
	mov rdx, rbx
	shr rdx, 30
	mov rcx, 0x1ff;9bit masking
	and rdx, rcx
	call set_entry_if_not_present
	mov rdi, [rdi+rdx*8]
	mov rcx, 0x0000fffffffff000
	and rdi, rcx

	;rdi=PDT
	mov rdx, rbx
	shr rdx, 21
	mov rcx, 0x1ff;9bit masking
	and rdx, rcx
	call set_entry_if_not_present
	mov rdi, [rdi+rdx*8]
	mov rcx, 0x0000fffffffff000
	and rdi, rcx


	;rdi=PT
	mov rdx, rbx
	shr rdx, 12
	mov rcx, 0x1ff;9bit masking
	and rdx, rcx
	mov [rdi+rdx*8], rax
	or byte [rdi+rdx*8], 3
	invlpg [rbx]
	
	pop rbx
	pop rdi
	pop rdx
	pop rcx
	ret

;in rdi page table
;in rdx index
set_entry_if_not_present:
	push rax
	test byte [rdi+rdx*8], 1;test present bit
	jnz .no_alloc
	xor rax, rax
	call lmode_alloc_frame
	call lmode_zero_4k
	mov [rdi+rdx*8], rax
	or byte [rdi+rdx*8], 3
.no_alloc:
	pop rax
	ret
	


;rax -> in ptr
lmode_zero_4k:
	push rax
	push rcx
	mov rcx, 512
.lwrite_zero:
	mov qword [rax], 0
	add rax, 8
	loop .lwrite_zero
	pop rcx
	pop rax
	ret
	
;in rax -> virtual address
;out rax -> physical address(-1 = error)
virtual_to_physical_addr:
	push rbx
	push rcx
	push rdx

	;get 48 bit address
	mov rcx, 0x0000ffffffffffff
	and rax, rcx

	mov rbx, cr3
	and bx, 0xf000

	mov rdx, rax
	shr rdx, 39
	mov rcx, 0x1ff
	and rdx, rcx
	;rdx -> index of the PML4
	mov rbx, [rbx+rdx*8];get address of the PDPT
	test rbx, 1
	jz .vtpa_error
	mov rcx, 0x0000fffffffff000
	and rbx, rcx

	
	mov rdx, rax
	shr rdx, 30
	mov rcx, 0x1ff
	and rdx, rcx

	;rdx -> index of the PDPT
	mov rbx, [rbx+rdx*8];get address of the PDT
	test rbx, 1
	jz .vtpa_error
	mov rcx, 0x0000fffffffff000
	and rbx, rcx

	mov rdx, rax
	shr rdx, 21
	mov rcx, 0x1ff
	and rdx, rcx

	;rdx -> index of the PDT
	mov rbx, [rbx+rdx*8];get address of the PT
	test rbx, 1
	jz .vtpa_error
	mov rcx, 0x0000fffffffff000
	and rbx, rcx

	mov rdx, rax
	shr rdx, 12
	mov rcx, 0x1ff
	and rdx, rcx

	;rdx -> index of the PT
	mov rax, [rbx+rdx*8];get physical address
	test rax, 1
	jz .vtpa_error
	mov rcx, 0x0000fffffffff000
	and rax, rcx

	jmp .vtpa_exit
.vtpa_error:
	mov rax, -1
.vtpa_exit:
	pop rdx
	pop rcx
	pop rbx
	ret

