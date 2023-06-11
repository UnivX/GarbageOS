#pragma once
#include <stdint.h>

/*BYTES MUL*/
#define KB 1024
#define MB 1024*1024
#define GB MB*KB
#define TB GB*KB

#define VBE_VADDR (void*)0xffff800000000000
#define VBE_VADDR_SIZE 1*GB

/*GENERAL ERROR CODES used in KPANIC*/
enum ErrorCodes{
	FRAME_ALLOCATOR_ERROR = 1,
	VBE_ERROR = 2,
	FREE_MEM_BOOTLOADER_ERROR = 3,
	KIO_ERROR = 4,
	FAILED_ASSERT = 5
};

//a bidimensional integer vector
typedef struct Vector2i{
	int64_t x, y;
} Vector2i;

#define KASSERT(x) if(!(x)) kpanic(FAILED_ASSERT);
