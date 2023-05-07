bits 64 
;in rax -> physical address
;in rbx -> virtual address
mmap:
	push rcx
	push rdx
	push rdi
	mov rdi, cr3
	and di, 0xf000
	;rdi used to contain the page map table
	;rdx used to calculate the index
	;rcx used for masking
	;48 bit paging so we do not use the first 16 bit
	;for other info search for 'canonical addresses in x86_64'

	;rdi=PML4 
	mov rdx, rbx
	shr rdx, 48-9
	and rdx, 0x1FF;9bit mask
	call set_entry_if_not_present
	mov rdi, [rdi+rdx]
	and di, 0xf000

	;rdi=PDPT
	mov rdx, rbx
	shr rdx, 48-9-9
	and rdx, 0x1FF;9bit mask
	call set_entry_if_not_present
	mov rdi, [rdi+rdx]
	and di, 0xf000

	;rdi=PDT
	mov rdx, rbx
	shr rdx, 48-9-9-9
	and rdx, 0x1FF;9bit mask
	call set_entry_if_not_present
	mov rdi, [rdi+rdx]
	and di, 0xf000


	;rdi=PT
	mov rdx, rbx
	shr rdx, 48-9-9-9-9
	and rdx, 0x1FF;9bit mask
	mov [rdi+rdx], rax
	or byte [rdi+rdx], 3
	
	pop rdi
	pop rdx
	pop rcx
	ret

;in rdi page table
;in rdx index
set_entry_if_not_present:
	push rax
	test byte [rdi+rdx], 1;test present bit
	jnz .no_alloc
	xor rax, rax
	call lmode_alloc_frame
	call lmode_zero_4k
	mov [rdi+rdx], rax
	or byte [rdi+rdx], 3
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
	
