#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "HAL/x86-64/gdt.h"
#include "frame_allocator.h"
#include "interrupt/interrupts.h"
#include "kio.h"
#include "elf.h"

//TODO: test the PIC
void page_fault(InterruptInfo info){
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

uint64_t kmain(){
	init_frame_allocator();
	set_up_firmware_layer();
	set_up_arch_layer();
	init_interrupts();
	enable_interrupts();
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
	for(int i = 0; i < 3; i++){
		print_uint64_dec(i);
		print(" - I'm a kernel\n");
	}

	asm volatile("int $0x40");

	return 0;
}
