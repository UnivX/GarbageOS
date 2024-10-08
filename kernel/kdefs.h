#pragma once
#include <stdint.h>
#include <stdbool.h>

/*COMPILATION OPTIONS*/
//#define DEBUG
//#define DO_TESTS
//#define PRINT_ALL_PAGE_FAULTS
//#define PRINT_ALL_VMMCACHE_SHOOTDOWNS
//#define PRINT_ALL_EOI
//#define PRINT_ALL_SPURIOUS_INTERRUPT

/*BYTES MUL*/
#define KB ((uint64_t)1024)
#define MB ((uint64_t)1024*1024)
#define GB MB*KB
#define TB GB*KB

/*KERNEL HEAP CONSTS*/
#define KERNEL_HEAP_SIZE 4*MB
#define KERNEL_HEAP_VMM_LOW_SECURITY_PADDING 16*KB
#define KERNEL_HEAP_VMM_MAX_SIZE 8*GB
#define KERNEL_HEAP_GROWTH_STEP 16*MB

/*KERNEL STACKS CONSTS*/
#define KERNEL_STACK_SIZE 1*MB 
#define KERNEL_STACK_EXPANSION_STEP 64*KB

//critical stack padding is the padding for page_fault and excetption stacks
#define CRITICAL_STACK_PADDING PAGE_SIZE
#define MINIMUM_STACK_PADDING CRITICAL_STACK_PADDING
#define PAGE_FAULT_STACK_SIZE 128*KB
#define EXCEPTION_STACK_SIZE 16*KB

#define PAGE_FAULT_ADDITIONAL_PAGE_LOAD 16

/*VMEM PARTITIONS*/
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
	VMM_ERROR = 12,
	APIC_ERROR = 13,
	GENERIC_ERROR = 14
};

//a bidimensional integer vector
typedef struct Vector2i{
	int64_t x, y;
} Vector2i;

#define S1(x) #x
#define S2(x) S1(x)
#define __LOCATION__ __FILE__ " : " S2(__LINE__) 
#define KASSERT(x) if(!(x)) kpanic_with_msg(FAILED_ASSERT, ""__LOCATION__);

//suppress compiler unused warning
#define UNUSED(x) (void)(x) 
