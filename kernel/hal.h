#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* HAL dependencies:
 * frame_allocator.h - it must implement an error if called before the initialization
 * 
 */

/*
 * HAL structs:
 */
/*if the size is 0 ignore the range*/
typedef struct PhysicalMemoryRange{
	uint64_t start_address, size;
} PhysicalMemoryRange;

typedef struct FreePhysicalMemoryStruct{
	size_t number_of_ranges;
	PhysicalMemoryRange* free_ranges;
} FreePhysicalMemoryStruct;


/*
 * HAL interface:
 */

//fatal error
void kpanic(volatile uint64_t error_code);

//map a virtual address to a physical address
void mmap(volatile void* vaddr, volatile void* paddr, uint8_t flags);

//this function is called at the start of the kmain 
//every call made to the HAL with the exception of kpanic
//must be made after this function call
void set_up_arch_layer();
bool is_hal_arch_initialized();

/*return a sorted array of free physical memory ranges
 * used to initialize the frame allocator
 * the range may have a size of zero
 */
FreePhysicalMemoryStruct free_mem_bootloader();
//return the last address for the last usable register size chunk of data
uint64_t get_last_address();
uint64_t get_total_usable_RAM_size();
//return a range of identity mapped address set up by the bootloader
PhysicalMemoryRange get_bootstage_indentity_mapped_RAM();


/*--------------x86-64 + BIOS default defines--------------*/
#define REGISTER_SIZE_BYTES 8
#include "HAL/x86-64/x64-memory.h"
