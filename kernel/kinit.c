#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "interrupt/interrupts.h"
#include "mem/frame_allocator.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "elf.h"

//called before the kmain by the early_kmain
//return the kernel stack pointer
//the stack present when kinit is called is small
void* kinit(){
	//at the moment we have a minimal no interrupt basic paging mapping set up from the bootloader
	init_frame_allocator();
	set_up_firmware_layer();
	early_set_up_arch_layer();
	init_interrupts();
	enable_interrupts();
	
	//SETUP MEMORY
	void* old_paging_struct = get_active_paging_structure();
	
	//create new paging_structure;
	void* new_paging_struct = create_empty_kernel_paging_structure();
	initialize_kernel_VMM(new_paging_struct);
	
	//identity map memory
	FreePhysicalMemoryStruct physical_mem = get_ram_space();
	for(uint64_t i = 0; i < physical_mem.number_of_ranges; i++){
		uint64_t paddr_start = physical_mem.free_ranges[i].start_address;
		uint64_t size = physical_mem.free_ranges[i].size;

		//if null size skipp
		if(size == 0)
			continue;

		//remove the NULL page
		if(paddr_start == 0){
			paddr_start = PAGE_SIZE;
			size -= PAGE_SIZE;
		}
		KASSERT(identity_map((void*)paddr_start, size) != NULL);
	}
	
	//map the kernel elf image
	ElfHeader* header = get_kernel_image();
	KASSERT(elf_validate_header(header));

	const uint64_t max_segments = 32;
	ElfLoadedSegment segments[max_segments];
	uint64_t number_of_segments = elf_get_number_of_loaded_entries(header);
	KASSERT(number_of_segments <= max_segments);
	elf_get_loaded_entries(header, segments, max_segments);
	for(uint64_t i = 0; i < number_of_segments; i++){
		uint16_t segments_flags = PAGE_PRESENT;

		uint64_t ssize = segments[i].size;
		if(ssize % PAGE_SIZE != 0)
			ssize += PAGE_SIZE - (ssize % PAGE_SIZE);

		if(segments[i].writeable)
			segments_flags |= PAGE_WRITABLE;

		KASSERT(copy_memory_mapping_from_paging_structure(old_paging_struct, segments[i].vaddr, ssize, segments_flags) != NULL);
	}


	//remove old bootloader paging_structure;
	set_active_paging_structure(new_paging_struct);
	delete_paging_structure(old_paging_struct);
	
	//allocate the heap
	kheap_init();

	//enable heap growth
	enable_kheap_growth();
	
	//allocate stack
	VMemHandle stack_mem = allocate_kernel_virtual_memory(KERNEL_STACK_SIZE, VM_TYPE_STACK, 16*KB, 64*MB);

	//the new stack is still not setted up but the memory is fully working
	set_up_arch_layer();

	return get_vmem_addr(stack_mem)+get_vmem_size(stack_mem);//return the stack top
}
