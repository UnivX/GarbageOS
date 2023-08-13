.section .text
.altmacro 

.macro generate_entry
.quad isr_stub_\@
.endm

.global isr_stub_table
.global isr_stub_table_end

isr_stub_table:
.rept 256
generate_entry
.endr
isr_stub_table_end:


#save a minimal context that may be changed by the kernel code
#based on what the SYSTEM V ABI says
.macro push_context
push %rax
push %rdi
push %rsi
push %rdx
push %rcx
push %r8
push %r9
push %r10
push %r11
.endm

.macro pop_context
pop %r11
pop %r10
pop %r9
pop %r8
pop %rcx
pop %rdx
pop %rsi
pop %rdi
pop %rax
.endm

.macro isr_stub_err n
isr_stub_\n:
	push_context
	pop %rsi
	mov $\n, %rdi
	cld
	call interrupt_routine
	pop_context
	iretq
.endm

.macro isr_stub_no_err n
isr_stub_\n:
	push_context
	mov $\n, %rdi
	mov $0, %rsi
	cld
	call interrupt_routine
	pop_context
	iretq
.endm

isr_stub_no_err 0 
isr_stub_no_err 1 
isr_stub_no_err 2 
isr_stub_no_err 3 
isr_stub_no_err 4 
isr_stub_no_err 5 
isr_stub_no_err 6 
isr_stub_no_err 7 
isr_stub_err 8 
isr_stub_no_err 9 
isr_stub_err 10 
isr_stub_err 11 
isr_stub_err 12 
isr_stub_err 13 
isr_stub_err 14 
isr_stub_no_err 15 
isr_stub_no_err 16 
isr_stub_err 17 
isr_stub_no_err 18 
isr_stub_no_err 19 
isr_stub_no_err 20 
isr_stub_no_err 21 
isr_stub_no_err 22 
isr_stub_no_err 23 
isr_stub_no_err 24 
isr_stub_no_err 25 
isr_stub_no_err 26 
isr_stub_no_err 27 
isr_stub_no_err 28 
isr_stub_no_err 29 
isr_stub_err 30 
isr_stub_no_err 31 

.set i, 32
.rept 255-31
	isr_stub_no_err %i
	.set i, i + 1
.endr