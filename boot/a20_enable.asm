enable_a20:
	push eax
	;check if a20 bios functions are supported
	mov ax, 2403h
	int 15h
	jb .fast_a20
	cmp ah, 0
	jnz .fast_a20

	;check if a20 is enabled via bios
	mov ax, 2402h
	int 15h
	jb .int_enable
	cmp ah, 0
	jnz .int_enable

	cmp al, 1
	jz .a20_enabled

.int_enable:

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

.fast_a20:
	mov si, a20_fast_msg
	call printc

	in al, 0x92
	test al, 2
	jnz .after
	or al, 2
	and al, 0xFE
	out 0x92, al
.after:	
	jmp .a20_enabled

.a20_failed_check:
	mov si, a20_fail_check_msg
	call printc
	jmp hang

a20_err db "a20 err", 13, 10, 0
a20_good db "a20 good", 13, 10, 0
a20_fast_msg db "a20 fast", 13, 10, 0
a20_fail_check_msg db "a20 failed checking", 13, 10, 0
