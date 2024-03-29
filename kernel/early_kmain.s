.section .bss
.align 16
stack_bottom:
.skip 1024*64# 64KiB
stack_top:
 
/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:
	movabs $stack_top, %rsp
	cld
	call kinit

	mov %rax, %rsp
	cld
	call kmain
	mov $0xc0ffebabe, %rcx
	cli
.h:	hlt
	jmp .h
 
/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
