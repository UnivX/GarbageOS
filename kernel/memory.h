#pragma once
#include <stdint.h>
#include <stddef.h>
#include "misc.h"


#define PAGE_SIZE 4096
/*paging flags*/
#define PAGE_PRESENT 1
#define PAGE_WRITABLE 2

//boot data
extern volatile void* boot_data;

//frame allocator data from the bootloader
typedef struct frame_data{
	uint32_t memory_map_item_offset;
	uint32_t first_frame_address;
}__attribute__((packed)) frame_data;


//single item result of the call of the BIOS function INT 0x15, EAX = 0xE820
//done by the bootloader
typedef struct memory_map_item{
	uint64_t base_addr;
	uint64_t size;
	uint32_t type;
	uint32_t acpi;
}__attribute__((packed)) memory_map_item;


void* early_frame_alloc();

void memset(volatile void* dst, uint8_t data, size_t size);

void mmap(volatile void* vaddr, volatile void* paddr, uint8_t flags);
