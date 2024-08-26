/*
Virtual Memory Manager(VMM)
*/
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../kdefs.h"
#include "../hal.h"
#include "../util/sync_types.h"
#include "../interrupt/interrupts.h"

#define VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE 64
#define COPY_FLAGS_ON_MMAP_COPY 0
#define MEMORY_MAP_PADDING PAGE_SIZE*4

//#define FREEZE_ON_PAGE_FAULT

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

typedef VirtualMemoryDescriptor* VMemHandle;

typedef struct PageFaultInfo{
	uint64_t page_error;
	void* fault_address;
	VMemHandle fault_vmem;
	InterruptInfo interrupt_info;
	bool page_not_present;
} PageFaultInfo;

//TODO: is_kernel_virtual_memory
bool is_kernel_virtual_memory(VirtualMemoryDescriptor descriptor);

typedef struct VirtualMemoryManager{
	VirtualMemoryDescriptor* vm_list_head;
	void* kernel_paging_structure;
	spinlock lock;
} VirtualMemoryManager;


void initialize_kernel_VMM(void* paging_structure);
const void* get_kernel_VMM_paging_structure();//sync
//may return NULL on failure
VMemHandle identity_map(void* paddr, uint64_t size);//sync
void load_identity_map_pages(void* paddr, uint64_t size, VMemHandle handle);//sync
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
VirtualMemoryType get_vmem_type(VMemHandle handle);
//if page_flags == COPY_FLAGS_ON_MMAP_COPY then the flags are simply copied
void debug_print_kernel_vmm();
//return the new cutted descriptor

//also if it's inside the padding
VMemHandle getHandleFromAddress(void* addr);
