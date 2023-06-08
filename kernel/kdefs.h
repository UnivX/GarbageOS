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
};
