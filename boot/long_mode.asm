bits 16
set_up_long_mode:
	cli
	lgdt [protected_mode_gdtr]
	mov eax, cr0
	or al, 1 ;set Protection Enabled flag
	mov cr0, eax
	jmp 0x08:.protected_mode
	call disable_nmi
.protected_mode:
	bits 32 ; Reload data segment registers:
	mov eax, 0x10
	mov ds, eax
	mov ss, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	mov cl, 64

	;setup the paging for long mode
	;page map level 4
	call pmode_alloc_frame
	mov [ds:pml4_physical_addr], eax
	call pmode_zero_4k

	;page directory pointer table
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the page directory pointer table as the frist entry for PML4
	mov ebx, [ds:pml4_physical_addr]
	mov [ds:ebx], eax
	or byte [ds:ebx], 3;set page present bit and read/write bit
	;save in ebx the PDPT
	mov ebx, eax

	;page directory table
	call pmode_alloc_frame
	call pmode_zero_4k
	;set the page directory table as the first entry of the PDPT
	mov [ds:ebx], eax
	or byte [ds:ebx], 3
	;save the PDT in ebx
	mov ebx, eax

	;map 1 GB of mem
	mov edx, 3
	mov ecx, 0
.fill_PDT:
	call pmode_alloc_frame
	call pmode_zero_4k
	mov [ebx+ecx*8], eax
	push ecx
	mov ecx, 512
.fill_PT:
	mov [ds:eax], edx
	add edx, 4096
	add eax, 8
	loop .fill_PT
	pop ecx
	or byte [ebx+ecx*8], 3
	inc ecx
	cmp ecx, 512
	jne .fill_PDT


	; Enable PAE
	mov edx, cr4
	or edx, (1 << 5)
	mov cr4, edx
 
	; Set LME (long mode enable)
	mov ecx, 0xC0000080
	rdmsr
	or eax, (1 << 8)
	wrmsr

	mov eax, [ds:pml4_physical_addr]
	mov cr3, eax

	;Enable paging
	or ebx, (1 << 31)+1
	mov cr0, ebx

	;set the right gdt
	lgdt [long_mode_gdtr]
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	jmp 0x08:.long_mode
.long_mode:
	bits 64
	mov rbx, 0xffffff8000000000
	mov rax, rbx
	call mmap

	mov rax, 0xC0FFEBABE
	;set the IDT limit to zero so if there is an NMI it will triple fault
	lidt [fake_idtr]
	call lmode_enable_nmi

.lmode_hang:
	hlt
	jmp .lmode_hang


bits 32
;eax -> in ptr
pmode_zero_4k:
	push ecx
	push eax
	mov ecx, 1024
.pwrite_zero:
	mov dword [ds:eax], 0
	add eax, 4
	loop .pwrite_zero
	pop eax
	pop ecx
	ret



protected_mode_gdt:
	dq 0x0000000000000000;NULL
	db 0xff, 0xff, 0, 0, 0, 10011010b, 11001111b, 0;code segment, 32bit mode
	db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0;data segment, 32bit mode
protected_mode_gdt_end:

protected_mode_gdtr:
	dw protected_mode_gdt_end-protected_mode_gdt-1
	dd protected_mode_gdt




long_mode_gdt:
	dq 0x0000000000000000;NULL
	db 0xff, 0xff, 0, 0, 0, 10011010b, 10101111b, 0;code segment, 64bit mode
	db 0xff, 0xff, 0, 0, 0, 10010010b, 10101111b, 0;data segment, 64bit mode
long_mode_gdt_end:

long_mode_gdtr:
	dw long_mode_gdt_end-long_mode_gdt-1
	dd long_mode_gdt

fake_idtr:
	dq 0
	dq 0

pml4_physical_addr dq 0

%include "mmap.asm"

bits 16
