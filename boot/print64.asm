print_rax:
    pusha
	push rax
	push rax

    mov ah, 0x0e
    mov al, '0'
    int 0x10

    mov ah, 0x0e
    mov al, 'x'
    int 0x10

    pop rax
    push rax
    shr eax, 16
    call print_ax
    pop eax
    call print_ax

	pop eax
	popa
	ret
