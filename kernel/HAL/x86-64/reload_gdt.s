.global _gdt_flush 

_gdt_flush:
	mov %rdi, %rax
	lgdt (%rax)
    mov $0x20, %rax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

	sub $16, %rsp
	movq $0x10, 8(%rsp)
	movabsq $flush, %rax
	mov %rax, (%rsp)
	lretq
flush:
	ret
