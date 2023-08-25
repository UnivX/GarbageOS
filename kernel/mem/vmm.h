/*
Virtual Memory Manager(VMM)
*/
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../kdefs.h"
#include "../hal.h"

#define VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE 64
#define COPY_FLAGS_ON_MMAP_COPY 0
#define MEMORY_MAP_PADDING PAGE_SIZE*4

//#define HALT_PAGE_FAULT

typedef enum VirtualMemoryType{
	VM_TYPE_FREE,
	VM_TYPE_STACK,
	VM_TYPE_HEAP,
	VM_TYPE_GENERAL_USE,
	VM_TYPE_IDENTITY_MAP_FREE,
	VM_TYPE_IDENTITY_MAP,
	VM_TYPE_MEMORY_MAPPING
} VirtualMemoryType;

typedef struct VirtualMemoryDescriptor{
	void* start_vaddr;
	uint64_t size_bytes;
	uint64_t upper_padding;
	uint64_t lower_padding;
	VirtualMemoryType type;
	struct VirtualMemoryDescriptor* next;
	struct VirtualMemoryDescriptor* prev;
	bool is_from_heap;//is this from the heap allocator or from the bump allocator?
} VirtualMemoryDescriptor;

bool is_kernel_virtual_memory(VirtualMemoryDescriptor descriptor);

typedef struct VirtualMemoryManager{
	VirtualMemoryDescriptor* vm_list_head;
	void* kernel_paging_structure;
	//future mutex
} VirtualMemoryManager;

typedef VirtualMemoryDescriptor* VMemHandle;

void initialize_kernel_VMM(void* paging_structure);
VMemHandle identity_map(void* paddr, uint64_t size);
VMemHandle memory_map(void* paddr, uint64_t size, uint16_t page_flags);
VMemHandle allocate_kernel_virtual_memory(uint64_t size, VirtualMemoryType type, uint64_t upper_padding, uint64_t lower_padding);
VMemHandle copy_memory_mapping_from_paging_structure(void* src_paging_structure, void* vaddr, uint64_t size, uint16_t page_flags);

/*
this function is available only for these Virtual Memory types
	VM_TYPE_STACK,
	VM_TYPE_HEAP,
	VM_TYPE_GENERAL_USE
*/
bool try_expand_vmem_top(VMemHandle handle, uint64_t size);

/*
this function is available only for these Virtual Memory types
	VM_TYPE_STACK,
	VM_TYPE_HEAP,
	VM_TYPE_GENERAL_USE
*/
bool try_expand_vmem_bottom(VMemHandle handle, uint64_t size);

bool deallocate_kernel_virtual_memory(VMemHandle handle);
uint64_t get_vmem_size(VMemHandle handle);
void* get_vmem_addr(VMemHandle handle);
//if page_flags == COPY_FLAGS_ON_MMAP_COPY then the flags are simply copied
void debug_print_kernel_vmm();
//return the new cutted descriptor

