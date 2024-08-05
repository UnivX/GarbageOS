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
#define LAST_VADDR (void*)0xffffffffffffffff
#define BIG_ENDIAN 0
#define LITTLE_ENDIAN 1
#define INVALID_ADDR (void*)~((uint64_t)(0))

/*
 * HAL arch specific defines
 */
#define ENDIANESS LITTLE_ENDIAN
#define PAGE_FAULT_INTERRUPT 0xe

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

typedef enum {
	MEMORYMAP_USABLE,
	MEMORYMAP_RESERVED,
	MEMORY_MAP_NON_VOLATILE,
	MEMORYMAP_BAD,
	MEMORYMAP_ACPI_TABLE,
	MEMORYMAP_ACPI_NVS
} MemoryMapType;

typedef struct MemoryMapRange{
	uint64_t start_address, size;
	MemoryMapType type;
} MemoryMapRange;

typedef struct MemoryMapStruct{
	MemoryMapRange* ranges;
	size_t number_of_ranges;
} MemoryMapStruct;

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

//if a frequency is 0MHz then the real frequency is unkwown
typedef struct CPUFrequencies{
	uint64_t core_base_freq; //in MHz
	uint64_t core_maximum_freq; //in MHz
	uint64_t core_crystal_clock_freq; //in MHz
	uint64_t bus_freq; //in MHz
	
} CPUFrequencies;

typedef bool InterruptState;

/*
 * HAL interface:
 */

//firmware dependant
void* acpi_RSDP();

//fatal error
void kpanic(volatile uint64_t error_code);

//map a virtual address to a physical address
void paging_map(volatile void* paging_structure, volatile void* vaddr, volatile void* paddr, uint16_t flags);
//set the memory map as invalid, the paging structure contain the old physical address
//but the memory mapping is no longer valid
void invalidate_paging_mapping(volatile void* paging_structure, volatile void* vaddr);
//if there is no mapping return INVALID_PADDR
PagingMapState get_physical_address(volatile void* paging_structure, volatile void* vaddr);

void set_active_paging_structure(volatile void* paging_structure);
void* get_active_paging_structure();

void* create_empty_kernel_paging_structure();
void delete_paging_structure(void* paging_structure);
uint64_t get_paging_mem_overhead(void* paging_structure);
//if new_flags == 0 then the flags are the same
void copy_paging_structure_mapping_no_page_invalidation(void* src_paging_structure, void* dst_paging_structure, void* start_vaddr, uint64_t size_bytes, uint16_t new_flags);

//this function is called at the start of the kmain 
//every call made to the HAL with the exception of kpanic
//must be made after this function call
void early_set_up_arch_layer();
void set_up_arch_layer();
void final_cpu_initialization(uint64_t number_of_logical_cores);
void set_up_firmware_layer();
bool is_hal_arch_initialized();


//gives back the number of logical cores that the kernel can use (they are the ones with a data strcuture set for them)
uint64_t get_number_of_usable_logical_cores();

/*return a sorted array of free physical memory ranges aligned to the PAGE_SIZE
 * used to initialize the frame allocator
 * the range may have a size of zero
 */
//get the free memory usable for allocation
FreePhysicalMemoryStruct free_mem_bootloader();

//get all the ram (also return the memory used by the boot loader, kernel and other reserved data)
FreePhysicalMemoryStruct get_ram_space();
//get memory map (sorted) from the firmware(uefi, bios, others??)
MemoryMapStruct get_memory_map();
uint64_t get_bootloader_memory_usage();
//return the last address for the last usable register size chunk of data
uint64_t get_last_address();
uint64_t get_total_usable_RAM_size();
//return a range of identity mapped address set up by the bootloader
PhysicalMemoryRange get_bootstage_indentity_mapped_RAM();

DisplayInterface get_firmware_display();

//return the image of the kernel that has been loaded by the bootloader
//NOTE: the address is physical
void* get_kernel_image();

void memory_fence();
void io_wait();

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

void outb(uint16_t port, uint8_t data);
void outw(uint16_t port, uint16_t data);
void outl(uint16_t port, uint32_t data);

void memset(volatile void* dst, uint8_t data, size_t size);
void memcpy(void* dst, const void* src, size_t size);

void enable_interrupts();
void disable_interrupts();
bool are_interrupts_enabled();
InterruptState disable_and_save_interrupts();
void restore_interrupt_state(InterruptState state);

void set_privilege_change_interrupt_stack(void* stack);
void halt();
void freeze_cpu();
bool cpu_has_msr();
void get_cpu_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void set_cpu_msr(uint32_t msr, uint32_t lo, uint32_t hi);
//may return invalid 0Hz frequencies
void get_cpu_frequencies(CPUFrequencies* out);
