#pragma once
#include <stdint.h>

/*BYTES MUL*/
#define KB ((uint64_t)1024)
#define MB ((uint64_t)1024*1024)
#define GB MB*KB
#define TB GB*KB

#define KERNEL_VADDR (void*)0xffffc00000000000

#define KERNEL_HEAP_SIZE 2*MB
#define KERNEL_STACK_SIZE 2*MB

#define KERNEL_VMEM_START (void*)0xffff800000000000
#define IDENTITY_MAP_VMEM_END (void*)(512*GB)

#define USER_VMEM_START IDENTITY_MAP_VMEM_END
#define USER_VMEM_END KERNEL_VMEM_START


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
	HEAP_OUT_OF_MEM = 9,
	HEAP_CORRUPTED = 10,
	NOT_IMPLEMENTED_ERROR = 11,
	VMM_ERROR = 12
};

//a bidimensional integer vector
typedef struct Vector2i{
	int64_t x, y;
} Vector2i;

#define KASSERT(x) if(!(x)) kpanic(FAILED_ASSERT);
