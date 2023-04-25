enable_a20:
	push eax
	;check if a20 bios functions are supported
	mov ax, 2403h
	int 15h
	jb .a20_error
	cmp ah, 0
	jnz .a20_error

	;check if a20 is enabled via bios
	mov ax, 2402h
	int 15h
	jb .a20_error
	cmp ah, 0
	jnz .a20_error

	cmp al, 1
	jz .a20_enabled

	;enable a20 via bios
	mov ax, 2403h
	int 15h
	jb .a20_error
	cmp ah, 0
	jnz .a20_error

.a20_enabled:
	mov si, a20_good
	call printc

	pop eax
	ret

.a20_error:
	mov si, a20_err
	call printc
	jmp hang

a20_err db "a20 err", 13, 10, 0
a20_good db "a20 good", 13, 10, 0
