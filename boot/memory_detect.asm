;https://wiki.osdev.org/Detecting_Memory_(x86)#Getting_an_E820_Memory_Map


;in -> ds:di ptr to buffer
;out -> ds:di first byte of free memory after the table

;uint32 size of list
;uint32 unused
;n list entry(24 bytes)
get_memory_map:
	push ebx
	push eax
	push edx
	push ecx

	mov [ds:md_edi_buffer], edi
	mov dword [ds:di], 0
	mov dword [ds:di+4],0
	add edi, 8

	xor ebx, ebx
.gmm_read_entry:
	mov dword [ds:di+20], 0;set the Extended Attributes as 0 if it is
	;not set by the bios call
	mov edx, 0x534D4150
	xor eax, eax
	mov eax, 0xE820
	mov ecx, 24
	int 15h
	jc .gmm_err

	add edi, 24
	push edi
	mov edi, [ds:md_edi_buffer]
	add dword [ds:di], 1
	pop edi

	cmp ebx, 0
	jz .gmm_end
	jmp .gmm_read_entry

.gmm_end:
	pop ecx
	pop edx
	pop eax
	pop ebx
	ret

.gmm_err:
	mov si, gmm_err_msg
	call printc
	jmp hang
memory_detect_data:
	md_edi_buffer dd 0
	gmm_err_msg db "error during the memory detection", 13,10,0
	gmm_dbg_msg db " debug gmm", 13,10,0
