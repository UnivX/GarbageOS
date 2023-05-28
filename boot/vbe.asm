; in -> ds:di 1024 byte buffer offset
init_vbe:
	pusha
	mov [ds:vbe_buffer_offset], di
	mov [ds:vbe_info_struct], di
	add di, 512
	mov [ds:vbe_mode_info_struct], di
	
	;get VESA BIOS information
	xor ax, ax
	mov es, ax
	mov di, [ds:vbe_info_struct]
	mov ax, 0x4f00
	int 0x10
	cmp ax, 0x004f
	jne vbe_init_error

	mov di, [ds:vbe_info_struct]
	mov ax, [ds:di+4]
	cmp ax, 0x200
	jb vbe_init_error

	popa
	ret

;in ax -> width
;in bx -> height
;in cx -> bits per pixel(bpp)
;out ax != 0 -> cannot load mode
try_load_mode:
	pusha
	mov [ds:vbe_width], ax
	mov [ds:vbe_height], bx
	mov [ds:vbe_bpp], cx
	xor ax, ax
	mov es, ax

	;set up the segment:offset for the modes at es:si
	mov bx, [ds:vbe_info_struct]
	mov ax, [ds:bx+14]
	mov si, ax
	mov ax, [ds:bx+14+2]
	mov fs, ax

.tlm_parse_mode:
	;load mode number in ds:vbe_mode
	mov dx, [fs:si]
	mov [ds:vbe_mode_number], dx
	add si, 2

	cmp dx, 0xffff
	je .tlm_error

	;read mode_info_struct
	push es
	mov di, [ds:vbe_mode_info_struct]
	;set up es:di to the vbe_mode_info_struct ptr
	mov ax, 0x4f01
	mov cx, [ds:vbe_mode_number];set cx as the mode number
	int 0x10
	cmp ax, 0x004f
	jne vbe_bios_int_error
	pop es

	;check if it's a linear buffer
	mov bx, [ds:vbe_mode_info_struct]
	mov al, [ds:bx]
	and al, 0x80
	jz .tlm_parse_mode

	mov al, [ds:vbe_bpp]
	cmp al, [ds:bx+25];bpp
	jne .tlm_parse_mode

	mov ax, [ds:vbe_width]
	cmp ax, [ds:bx+18];width
	jne .tlm_parse_mode

	mov ax, [ds:vbe_height]
	cmp ax, [ds:bx+20];height
	jne .tlm_parse_mode


	;found right mode
	;load it
	mov ax, 0x4f02
	;bx mode number plus LFB
	mov bx, (1<<14);LFB(Linear Frame Buffer) bit set
	or bx, [ds:vbe_mode_number];
	int 0x10
	cmp ax, 0x004f
	jne vbe_bios_int_error

	mov ax, 0
	jmp .tlm_end
.tlm_error:
	mov ax, 1
.tlm_end:
	mov [ds:vbe_temp_ret_value], ax
	popa
	xor ax, ax
	mov es, ax
	mov ax, [ds:vbe_temp_ret_value]
	ret

;cl = value to put in the buffer
fill_screen_grey_scale:
	push ebx
	push edx
	push eax
	push ecx

	mov bx, [ds:vbe_mode_info_struct]
	mov ebx, [ds:bx+40];get_frame_buffer_address in 32 bit address
	;write the first 12 bytes as 0xff(if 32 bit depth then 3 pixels, if 24 bit then 4 pixels)
	mov edx, 1024*768
	mov eax, [ds:vbe_bpp]
	shr eax, 3
	mul edx
.write_vbe_mem:
	mov [ds:ebx], cl
	add ebx, 1
	dec eax
	jnz .write_vbe_mem
	pop ecx
	pop eax
	pop edx
	pop ebx
	ret

vbe_init_error:
	xor ax, ax
	mov es, ax
	mov si, vbe_init_error_msg
	call printc
	jmp hang

vbe_bios_int_error:
	xor ax, ax
	mov es, ax
	mov si, vbe_error_bios_int_msg
	call printc
	jmp hang

vbe_data:
	vbe_buffer_offset dd 0
	vbe_info_struct dd 0
	vbe_mode_info_struct dd 0
	vbe_init_error_msg db "VBE 2.0+ BIOS extension not supported", 13, 10, 0
	vbe_error_bios_int_msg db "VBE error while using bios interrupt",13,10,0
	vbe2_signature db "VESA"
	vbe_bpp dd 0
	vbe_width dw 0
	vbe_height dw 0
	vbe_temp_ret_value dw 0
	vbe_mode_number dw 0
