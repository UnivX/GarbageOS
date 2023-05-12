bits 32

setup_paging:
	pushad
	;PML4
	call pmode_alloc_frame
	mov [ds:pml4_physical_addr], eax
	call pmode_zero_4k

	;PDPT
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the page directory pointer table as the frist entry for PML4
	mov ebx, [ds:pml4_physical_addr]
	mov [ds:ebx], eax
	or byte [ds:ebx], 3;set page present bit and read/write bit
	;save in ebx the PDPT
	mov ebx, eax

	;PDT
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the page directory table as the first entry of the PDPT
	mov [ds:ebx], eax
	or byte [ds:ebx], 3
	;save the PDT in ebx
	mov ebx, eax

	;PT
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the PT as the frist PDT entry
	mov [ds:ebx], eax
	or byte [ds:ebx], 3
	;save the PT in ebx
	mov ebx, eax

	mov ecx, 256
	mov edx, 3
.MB_map:
	mov [ebx], edx
	add ebx, 8
	add edx, 4096
	loop .MB_map

	;PDPT
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the page directory pointer table as the last entry for PML4
	mov ebx, [ds:pml4_physical_addr+(511*8)]
	mov [ds:ebx], eax
	or byte [ds:ebx], 3;set page present bit and read/write bit
	;save in ebx the PDPT
	mov ebx, eax
	

	popad
	ret

pml4_physical_addr dq 0

bits 16
