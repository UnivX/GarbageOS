;https://wiki.osdev.org/How_Do_I_Determine_The_Amount_Of_RAM
;in -> ds:di ptr to buffer
;out -> ds:di first byte of free memory after the table

;uint32 size of list
;uint32 unused
;n list entry(24 bytes)
get_memory_map:
	pusha

	mov [ds:md_edi_buffer], edi
	mov dword [ds:di], 0
	mov dword [ds:di+4],0

	xor ebx, ebx
.gmm_read_entry:
	mov dword [ds:di+8], 1
	mov dword [ds:di+12], 0
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
	popa
	ret

.gmm_err:
	popa
	mov si, gmm_err_msg
	call printc
	jmp hang
memory_detect_data:
	md_edi_buffer dd 0
	gmm_err_msg db "error during the memory detection", 13,10,0
	gmm_dbg_msg db " debug gmm", 13,10,0
