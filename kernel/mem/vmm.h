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

typedef enum VirtualMemoryType{
	VM_TYPE_FREE,
	VM_TYPE_IDENTITY_MAP_FREE,
	VM_TYPE_STACK,
	VM_TYPE_HEAP,
	VM_TYPE_GENERAL_USE,
	VM_TYPE_MEMORY_MAPPING,
	VM_TYPE_IDENTITY_MAP,
} VirtualMemoryType;

typedef struct VirtualMemoryDescriptor{
	void* start_vaddr;
	uint64_t size_bytes;
	uint64_t upper_padding;
	uint64_t lower_padding;
	VirtualMemoryType type;
	struct VirtualMemoryDescriptor* next;
	struct VirtualMemoryDescriptor* prev;
	bool is_from_heap;
} VirtualMemoryDescriptor;

bool is_kernel_virtual_memory(VirtualMemoryDescriptor descriptor);

typedef struct VirtualMemoryManager{
	VirtualMemoryDescriptor* task_vm_list_head;
	void* kernel_paging_structure;
	//future mutex
} VirtualMemoryManager;

void initialize_kernel_VMM(void* paging_structure);
bool identity_map(void* paddr, uint64_t size);
void* memory_map(void* paddr, uint64_t size, uint16_t page_flags);
void* allocate_kernel_virtual_memory(uint64_t size, VirtualMemoryType type, uint64_t upper_padding, uint64_t lower_padding);
//if page_flags == COPY_FLAGS_ON_MMAP_COPY then the flags are simply copied
bool copy_memory_mapping_from_paging_structure(void* src_paging_structure, void* vaddr, uint64_t size, uint16_t page_flags);
void debug_print_kernel_vmm();
//return the new cutted descriptor

/*
//allocate a new kernel stack
//return the stack top pointer
void* alloc_stack(uint64_t size);
void* alloc_kernel_heap();
*/
