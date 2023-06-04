#pragma once
#include <stdint.h>

/*BYTES MUL*/
#define KB 1024
#define MB 1024*1024
#define GB MB*KB
#define TB GB*KB

/*GENERAL ERROR CODES used in KPANIC*/
#define FRAME_ALLOCATOR_ERROR 1
#define VBE_ERROR 2
#define FREE_MEM_BOOTLOADER_ERROR 3

/*paging flags*/
#define PAGE_PRESENT 1
#define PAGE_WRITABLE 2
