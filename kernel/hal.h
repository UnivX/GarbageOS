#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "display_interface.h"

/* HAL dependencies:
 * frame_allocator.h - it must implement an error if called before the initialization
 * 
 */

/*
 * HAL common defines
 */

#define INVALID_ADDR (void*)~((uint64_t)(0))

/*
 * HAL arch specific defines
 */
/*--------------x86-64 + BIOS default defines--------------*/
/*paging common flags*/
#define PAGE_PRESENT 1
#define PAGE_WRITABLE 2
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_WRITE_THROUGH (1 << 3)
#define REGISTER_SIZE_BYTES 8
#include "HAL/x86-64/x64-memory.h"

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
 * paging state associated with a virtual address
 * the paddr field is equal to INVALID_ADDR when there is no memory mapping to it
 * if the memory mapping has been explicity invalidated but the address is still there
 * then paddr and flags fields will contain the old memory map values that had been invalidated
 */
typedef struct PagingMapState{
	void* paddr;
	uint16_t flags;
	bool mmap_present;
} PagingMapState;


/*
 * HAL interface:
 */

//fatal error
void kpanic(volatile uint64_t error_code);

//map a virtual address to a physical address
void mmap(volatile void* vaddr, volatile void* paddr, uint16_t flags);
//set the memory map as invalid, the paging structure contain the old physical address
//but the memory mapping is no longer valid
void invalidate_mmap(volatile void* vaddr);
//if there is no mapping return INVALID_PADDR
PagingMapState get_physical_address(volatile void* vaddr);

//this function is called at the start of the kmain 
//every call made to the HAL with the exception of kpanic
//must be made after this function call
void set_up_arch_layer();
void set_up_firmware_layer();
bool is_hal_arch_initialized();

/*return a sorted array of free physical memory ranges aligned to the PAGE_SIZE
 * used to initialize the frame allocator
 * the range may have a size of zero
 */
FreePhysicalMemoryStruct free_mem_bootloader();
//return the last address for the last usable register size chunk of data
uint64_t get_last_address();
uint64_t get_total_usable_RAM_size();
//return a range of identity mapped address set up by the bootloader
PhysicalMemoryRange get_bootstage_indentity_mapped_RAM();

DisplayInterface get_firmware_display();

void memory_fence();
