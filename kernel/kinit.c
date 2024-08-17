#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "interrupt/interrupts.h"
#include "mem/frame_allocator.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "elf.h"
#include "acpi/acpi.h"
#include "acpi/madt.h"
#include "interrupt/apic.h"
#include "kernel_data.h"

void setup_virtual_memory(){
	//SETUP MEMORY
	void* old_paging_struct = get_active_paging_structure();
	
	//create new paging_structure;
	void* new_paging_struct = create_empty_kernel_paging_structure();
	initialize_kernel_VMM(new_paging_struct);

	//DO NOT IDENTITY MAP THE FIRST PAGE(it's needed for the nullpointer exception)
	void* identity_map_start_addr = (void*)PAGE_SIZE;
	VMemHandle identity_map_handle = identity_map(identity_map_start_addr, (uint64_t)IDENTITY_MAP_VMEM_END-(uint64_t)identity_map_start_addr);
	KASSERT(identity_map_handle != NULL);

	//OLD IDENTITY MAPPING CODE
	//identity map usable physical memory
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
		load_identity_map_pages((void*)paddr_start, size, identity_map_handle);
		//KASSERT(identity_map((void*)paddr_start, size) != NULL);
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
}

//called before the kmain by the early_kmain
//return the kernel stack pointer
//the stack present when kinit is called is small
void* kinit(){
	//at the moment we have a minimal no interrupt basic paging mapping set up from the bootloader
	init_frame_allocator();
	set_up_firmware_layer();
	early_set_up_arch_layer();
	init_interrupts();//virtual memory requires the set up of interrupts for page faults

	setup_virtual_memory();

	
	
	//allocate the heap
	kheap_init();

	//enable heap growth
	enable_kheap_growth();
	

	//the new stack is still not setted up but the memory is fully working
	//now we initialize the CPU with a correct configuration
	//int this initialization we prepare the internal data structures only for one logical core
	set_up_arch_layer();
	acpi_init();//it requires the virtual memory

	//set up kernel data
	init_kernel_data();
	
	//now that we a basic system working we can find the number of logical cores via MADT and we can 
	//initialize the CPU data structures ecc ecc for multiple logical cores
	MADT* madt = get_MADT();
	uint64_t number_of_logical_cores = count_number_of_local_apic(madt);
	
	//register cpu
	CPUID bsp_cpuid = register_local_kernel_data_cpu();
	//init cpu
	init_cpu_data(number_of_logical_cores);
	init_cpu(bsp_cpuid);

	//this function enable and initialize the pic, the local apic of the BPS core and the IOAPICS
	//to work it requires the ACPI tables and the heap
	init_interrupt_controller_subsystems();
	init_local_interrupt_controllers();
	init_global_interrupt_controllers();

	enable_interrupts();

	//allocate stack
	VMemHandle stack_mem = allocate_kernel_virtual_memory(KERNEL_STACK_SIZE, VM_TYPE_STACK, 16*KB, 64*MB);

	//set local cpu_data
	LocalKernelData local_data = {stack_mem, get_logical_core_lapic_id()};
	set_local_kernel_data(bsp_cpuid, local_data);

	return get_vmem_addr(stack_mem)+get_vmem_size(stack_mem);//return the stack top
}
