#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "HAL/x86-64/gdt.h"
#include "frame_allocator.h"
#include "interrupt/interrupts.h"
#include "kio.h"

//TODO: test the PIC

void interrupt_print(uint64_t error){
	print("interrupt 0x40 called");
}

void page_fault(uint64_t error){
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
	halt();
}

uint64_t kmain(){
	init_frame_allocator();
	set_up_firmware_layer();
	set_up_arch_layer();
	init_interrupts();
	enable_interrupts();
	install_interrupt_handler(0x40, interrupt_print);
	install_interrupt_handler(0xe, page_fault);

	DisplayInterface display = get_firmware_display();
	display.init();
	if(display.info.height*display.info.width > BUFFER_SIZE)
		return -1;

	Color background_color = {0,0,0,255};
	Color font_color = {0,255,0,255};
	PSFFont font = get_default_PSF_font();
	init_kio(display, font, background_color, font_color);
	for(int i = 0; i < 100; i++){
		print_uint64_dec(i);
		print(" - I'm a kernel\n");
	}
	asm volatile("int $0x40");
	return 0;
}
