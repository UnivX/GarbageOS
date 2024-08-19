#include "../../hal.h"
#include "../../kio.h"
#define KPANIC_PRINT

void kpanic(volatile uint64_t error_code){
	//TODO: make the kernel panic indipendent from the kernel
#ifdef KPANIC_PRINT
	if(is_kio_initialized()){
		printf("[KERNEL PANIC] %u64\n", (uint64_t)error_code);
		kio_flush();
	}
#endif
	//put in rax and rbx the error_code, rcx=0xdeadbeef
	//never ending loop
	asm volatile("1: mov $0xdeadbeef, %%rcx\nmov %0, %%rax\nhlt\njmp 1b" : : "b"(error_code) : "rax", "rcx", "cc");
	return;
}

void kpanic_with_msg(volatile uint64_t error_code, const char* msg){
	//TODO: make the kernel panic indipendent from the kernel
#ifdef KPANIC_PRINT
	if(is_kio_initialized()){
		printf("[KERNEL PANIC] error code = %u64, msg = %s\n", (uint64_t)error_code, msg);
		kio_flush();
	}
#endif
	//put in rax and rbx the error_code, rcx=0xdeadbeef
	//never ending loop
	asm volatile("1: mov $0xdeadbeef, %%rcx\nmov %0, %%rax\nhlt\njmp 1b" : : "b"(error_code) : "rax", "rcx", "cc");
	return;
}
