#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "HAL/x86-64/gdt.h"
#include "mem/frame_allocator.h"
#include "interrupt/interrupts.h"
#include "mem/heap.h"
#include "mem/vmm.h"
#include "kio.h"
#include "elf.h"

//TODO: test the PIC
void page_fault(InterruptInfo info){
	if(!is_kio_initialized())
		kpanic(UNRECOVERABLE_PAGE_FAULT);

	uint64_t error = info.error;
	print("[PAGE FAULT] ");

	if(error & 1<<1)
		print("(write) ");
	else
		print("(read) ");

	if(!(error & 1))
		print("page not present ");
	else if(error & 1<<3)
		print("reserved write ");
	if(error & 1 << 4)
		print("caused by instruction fetch ");
	if(error & 1 <<5)
		print("caused by Protection Key ");
	if(error & 1 <<6)
		print("caused by Shadow Stack Access ");
	if(error & 1 <<15)
		print("caused by SGX ");

	uint64_t cr2;
	asm("mov %%cr2, %0" : "=r"(cr2) : : "cc");
	print("\n[PAGE FAULT ADDRESS] ");
	print_uint64_hex(cr2);
	print("\n");
	if(cr2 == 0)
		print("[PAGE FAULT] NULL POINTER DEREFERENCE\n");
	kpanic(UNRECOVERABLE_PAGE_FAULT);
}

void general_protection_fault(InterruptInfo info){
	print("[GENERAL PROTECTION FAULT] error: ");
	print_uint64_hex(info.error);
	print("\n");
	kpanic(UNRECOVERABLE_GPF);
}

void interrupt_print(InterruptInfo info){
	print("[INT ");
	print_uint64_dec(info.interrupt_number);
	print("] error: ");
	print_uint64_hex(info.error);
	print("\n");
}

void print_elf_info(){
	print("kernel image addr: ");
	print_uint64_hex((uint64_t)get_kernel_image());
	print("\n");
	ElfHeader* kernel_elf = (ElfHeader*)get_kernel_image();

	if(!elf_validate_header(kernel_elf)){
		print("elf image is not valid\n");
		return;
	}
	print("elf image is valid\n");

	uint16_t loaded_segments = elf_get_number_of_loaded_entries(kernel_elf);
	const size_t segments_size = 32;
	ElfLoadedSegment segments[segments_size];//max 32 segments
	elf_get_loaded_entries(kernel_elf, segments, segments_size);

	print("number of loaded segments: ");
	print_uint64_dec((uint64_t)loaded_segments);
	print("\n");
	if(loaded_segments > segments_size)
		print("displaying only the first 32 segments:\n");
	for(uint16_t i = 0; i < loaded_segments && i < segments_size; i++){
		print("Start: ");
		print_uint64_hex((uint64_t)segments[i].vaddr);
		print(" / Size: ");
		print_uint64_hex(segments[i].size);
		print("\n");
	}
}

void heap_stress_test(){
	uint64_t start_number_of_chunks = get_number_of_chunks_of_kheap();
	const int max = 100;
	void* allocated[max];
	size_t sizes[5] = {32, 34, 512, 129, 16};
	int sizes_idx = 0;

	for(int i = 0; i < max; i++){
		allocated[i] = kmalloc(sizes[sizes_idx]);
		sizes_idx = (sizes_idx+1) % 5;
	}

	int n_of_free = 0;
	uint32_t rand = 0x79a9f11c;
	uint32_t times = 500;
	while(n_of_free < 100 && times > 0){
		uint32_t next_idx = rand % max;
		if(allocated[next_idx] != NULL){
			n_of_free++;
			kfree(allocated[next_idx]);
			allocated[next_idx] = NULL;
		}
		rand = (rand ^ (0x672890f1+(~(rand ^ 0xb69f5cca) + n_of_free)) + rand-(~n_of_free) % (rand ^ 0x672890f1));
		times--;
	}
	for(int i = 0; i < max; i++)
		if(allocated[i] != NULL)
			kfree(allocated[i]);

	bool heap_corrupted = is_kheap_corrupted();
	if(get_number_of_chunks_of_kheap() == start_number_of_chunks && !heap_corrupted)
		print("HEAP stress test PASSED\n");
	else{
		print("HEAP stress test FAILED / heap chunks from ");
		print_uint64_dec(get_number_of_chunks_of_kheap());
		print(" to ");
		print_uint64_dec(start_number_of_chunks);
		print("\n");
		print("HEAP IS ");
		if(!heap_corrupted)
			print("NOT ");
		print("CORRUPTED\n");
	}
}

void initialize(){
	//at the moment we have a minimal no interrupt basic paging mapping set up from the bootloader
	init_frame_allocator();
	set_up_firmware_layer();
	set_up_arch_layer();
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
	VMemHandle kheap_mem = allocate_kernel_virtual_memory(KERNEL_HEAP_SIZE, VM_TYPE_HEAP, 8*GB, 16*KB);
	kheap_init(get_vmem_addr(kheap_mem), KERNEL_HEAP_SIZE);

	return;
}

uint64_t kmain(){
	initialize();
	install_default_interrupt_handler(interrupt_print);
	install_interrupt_handler(0xe, page_fault);
	install_interrupt_handler(0xd, general_protection_fault);

	DisplayInterface display = get_firmware_display();
	display.init();
	if(display.info.height*display.info.width > BUFFER_SIZE)
		return -1;
	Color background_color = {0,0,0,255};
	Color font_color = {0,255,0,255};
	PSFFont font = get_default_PSF_font();
	init_kio(display, font, background_color, font_color);
	print_elf_info();

	print("free allocable memory(MB): ");
	print_uint64_dec(get_number_of_free_frames() * PAGE_SIZE / MB);
	print("\n");


	print("free allocable memory after heap allocation(MB): ");
	print_uint64_dec(get_number_of_free_frames() * PAGE_SIZE / MB);
	print("\n");

	heap_stress_test();

	asm volatile("int $0x40");
	debug_print_kernel_vmm();

	finalize_kio();
	return 0;
}
