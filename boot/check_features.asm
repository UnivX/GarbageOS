check_cpuid:
	push eax
	push ebx
	pushfd

	;try to flip funny bit
	pushfd
	mov bx, sp
	xor dword [ds:bx], 0x00200000
	popfd

	;eax now contains the eflags that maybe is flipped
	pushfd
	pop eax

	;the stack top contains the saved original eflags
	;check if the funny bit is flipped
	mov bx, sp
	cmp eax, [ds:bx]
	je .cpuid_not_supported

	;restore the eflags
	popfd
	pop ebx
	pop eax
	ret

.cpuid_not_supported:
	mov si, cpuid_not_supported_msg
	call printc
	jmp hang

cpuid_not_supported_msg db "cpuid not supported", 13, 10, 0




check_long_mode:
	push eax
	push edx
	mov eax, 0x80000001
	cpuid
	test edx, (1 << 29)
	jz .no_64bit_error
	pop edx
	pop eax
	ret


.no_64bit_error:
	mov si, no_64bit_msg
	call printc
	jmp hang

no_64bit_msg db "cant boot up, the system isn't x86_64", 13, 10, 0
