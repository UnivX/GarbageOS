#include "../../hal.h"
#include "../../kio.h"
#define KPANIC_PRINT

void kpanic(volatile uint64_t error_code){
	//TODO: make the kernel panic indipendent from the kernel
#ifdef KPANIC_PRINT
	if(is_kio_initialized()){
		print("[KERNEL PANIC] ");
		print_uint64_dec(error_code);
	}
#endif
	//put in rax and rbx the error_code, rcx=0xdeadbeef
	//never ending loop
	asm volatile("1: mov $0xdeadbeef, %%rcx\nmov %0, %%rax\nhlt\njmp 1b" : : "b"(error_code) : "rax", "rcx", "cc");
	return;
}

