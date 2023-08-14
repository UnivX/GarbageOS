#pragma once
#include <stdint.h>

/*BYTES MUL*/
#define KB ((uint64_t)1024)
#define MB ((uint64_t)1024*1024)
#define GB MB*KB
#define TB GB*KB

#define KERNEL_VADDR (void*)0xffffc00000000000

#define VBE_VADDR (void*)0xffff800000000000
#define VBE_VADDR_SIZE 1*GB

#define KERNEL_ADDITIONAL_STACKS_VADDR (void*)0xffff900000000000
#define KERNEL_ADDITIONAL_STACK_SIZE 2*GB

#define INTERRUPT_STACK_SIZE 64*KB
#define SYSTEM_CALL_STACK_SIZE 64*KB

/*GENERAL ERROR CODES used in KPANIC*/
enum ErrorCodes{
	FRAME_ALLOCATOR_ERROR = 1,
	VBE_ERROR = 2,
	FREE_MEM_BOOTLOADER_ERROR = 3,
	KIO_ERROR = 4,
	FAILED_ASSERT = 5,
	STACK_SMASHING = 6,
	UNRECOVERABLE_GPF = 7,
	UNRECOVERABLE_PAGE_FAULT = 8,
	EARLY_ALLOCATOR_ERROR = 9
};

//a bidimensional integer vector
typedef struct Vector2i{
	int64_t x, y;
} Vector2i;

#define KASSERT(x) if(!(x)) kpanic(FAILED_ASSERT);
