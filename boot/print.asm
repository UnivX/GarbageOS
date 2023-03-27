print_eax:
    pusha
	push eax
	push eax

    mov ah, 0x0e
    mov al, '0'
    int 0x10

    mov ah, 0x0e
    mov al, 'x'
    int 0x10

    pop eax
    push eax
    shr eax, 16
    call print_ax
    pop eax
    call print_ax

	pop eax
	popa
	ret

print_ax:;procedure to print the ax hex value
    pusha
    push ax

    pop ax
    push ax
    shr ax, 8
    call print_al
    pop ax
    call print_al

    popa
    ret
print_al:;procedure to print the al hex value
    pusha
    push ax
    and al, 0xf0
    shr al, 4
    add al, 0x30
    cmp al, 0x3a

    jnae not_alphabet1
    add al, 7
not_alphabet1:
    mov ah, 0x0e
    int 0x10

    pop ax
    and al, 0x0f
    add al, 0x30
    cmp al, 0x3a

    jnae not_alphabet2
    add al, 7
not_alphabet2:
    mov ah, 0x0e
    int 0x10

    popa
    ret
printc:;procedure to print a null terminated string
    pusha
prnt:
    xor ax, ax
    xor bx, bx
    mov ds, ax
    mov ah, 0x0e
    mov al, [ds:si]
    cmp bl, al
    jz printdone
    inc si
    int 0x10
    jmp prnt
printdone:
    popa
    ret
