; in -> ds:di 1024 byte buffer offset
init_vbe:
	pusha
	mov [ds:vbe_buffer_offset], di
	mov [ds:vbe_info_struct], di
	add di, 512
	mov [ds:vbe_mode_info_struct], di
	
	;get VESA BIOS information
	mov ax, 0x4f00
	mov di, [ds:vbe_info_struct]
	int 0x10
	cmp ax, 0x004f
	jne vbe_init_error

	mov ax, [ds:vbe_info_struct+4]
	cmp ax, 0x200
	jb vbe_init_error

	popa
	ret

vbe_init_error:
	mov si, vbe_init_error_msg
	call printc

vbe_data:
	vbe_buffer_offset dd 0
	vbe_info_struct dd 0
	vbe_mode_info_struct dd 0
	vbe_init_error_msg db "VBE 2.0+ BIOS extension not supported", 13, 10, 0
	vbe2_signature db "VESA"
