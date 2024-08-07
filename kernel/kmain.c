#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "interrupt/interrupts.h"
#include "mem/frame_allocator.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "kio.h"
#include "elf.h"
#include "acpi/acpi.h"
#include "acpi/madt.h" 
#include "interrupt/apic.h"
#include "interrupt/ioapic.h"
#include "timer/pit.h"

#include "test/kheap_test.h"
//first usable intel CPU in history for this kernel : XEON Nocona (Jun 2004)
//first usable AMD CPU in history for this kernel? : Opteron (Apr 2003).

//TODO: fully implement memset, memcpy, memmove and memcmp
//TODO: check APIC existence before usage
//TODO: print PIC state
//TODO: use the PIC's ISR to check when to send a PIC's EOI
//TODO: check the PIC spurious interrupt handling
//TODO: add PAT
//TODO: add lazy page mapping
//TODO: implement uefi bootloader for sweet acpi 2.0 tables
//TODO: test acpi 2.0
//TODO: clear all paging map levels when deallocating virtual memory
//TODO: prevent nested page fault

PIT pit;

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
	print("KERNEL IMAGE INFO\n");
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

volatile bool f = false;
void stack_overflower(int s){
	if(s != 0)
		stack_overflower(s-1);
	f = !f;
}

uint64_t kmain(){
	install_default_interrupt_handler(interrupt_print);
	install_interrupt_handler(0xd, general_protection_fault);

	DisplayInterface display = get_firmware_display();
	display.init(display);
	Color background_color = {0,0,0,255};
	Color font_color = {0,255,0,255};
	PSFFont font = get_default_PSF_font();
	init_kio(display, font, background_color, font_color);

	print("initializing PIT timer\n");
	pit = create_PIT(0x41);
	uint64_t pit_real_freq = initialize_PIT_timer(&pit, 1000);
	print("PIT frequency: ");
	print_uint64_dec(pit_real_freq);
	print("\n");

	print_elf_info();

	print("\nMEMORY MAP:\n");
	MemoryMapStruct memMap = get_memory_map();
	for(size_t i = 0; i < memMap.number_of_ranges; i++){
		const char* type = 							"[   UNKNOWN  ]";
		switch(memMap.ranges[i].type){
			case MEMORYMAP_BAD: type = 				"[     BAD    ]";
				break;
			case MEMORYMAP_USABLE: type = 			"[     RAM    ]";
				break;
			case MEMORYMAP_ACPI_NVS: type = 		"[  ACPI NVS  ]";
				break;
			case MEMORYMAP_RESERVED: type = 		"[  RESERVED  ]";
				break;
			case MEMORYMAP_ACPI_TABLE: type = 		"[ ACPI TABLE ]";
				break;
			case MEMORY_MAP_NON_VOLATILE: type = 	"[NON VOLATILE]";
				break;
		}
		print(type);
		print(" ");
		print_uint64_hex(memMap.ranges[i].start_address);
		print(" / ");
		print_uint64_hex(memMap.ranges[i].start_address+memMap.ranges[i].size);
		print("\n");
	}
	print("\n");
	debug_print_kernel_vmm();

	
	//return 0;
	print("\nSleeping 10s\n");
	PIT_wait_ms(&pit, 10000);
	
	//for(int i = 0; i < 4000; i++)
		//io_wait();

#ifdef DO_TESTS
	print("\n-------------------STARTING TESTS-------------------\n");
	print("starting stack overflower\n");
	stack_overflower(4000);
	asm volatile("int $0x40");
	heap_stress_test();
#endif
	print("\n-------------------PRINTING MEMORY USAGE-------------------\n");

	uint64_t kernel_bootloader_overhead = get_total_usable_RAM_size()-(get_number_of_free_frames()*PAGE_SIZE);
	print("kernel + bootloader memory overhead: ");
	print_uint64_dec(kernel_bootloader_overhead / MB);
	print(" MiB\n");

	print("kernel paging memory overhead: ");
	print_uint64_dec(get_paging_mem_overhead(get_active_paging_structure()) / MB);
	print(" MiB\n");

	print("kernel frame allocator overhead: ");
	print_uint64_dec(get_frame_allocator_mem_overhead() / MB);
	print(" MiB\n");

	print("kernel heap start size: ");
	print_uint64_dec(KERNEL_HEAP_SIZE / MB);
	print(" MiB\n");

	print("kernel heap actual size: ");
	print_uint64_dec(get_kheap_total_size() / MB);
	print(" MiB\n");

	print("kernel stack size: ");
	print_uint64_dec(KERNEL_STACK_SIZE / MB);
	print(" MiB\n");

	print("System Total RAM: ");
	print_uint64_dec(get_total_usable_RAM_size() / MB);
	print(" MiB\n");

	print_acpi_rsdt();

	print("number of logical cores: ");
	print_uint64_dec(get_number_of_usable_logical_cores());
	print("\n");

	print( madt_has_pic(get_MADT()) ? "the PC has PIC\n" : "the PC doesnt have PIC\n");

	//do a self interrupt with the local apic
	send_IPI_by_destination_shorthand(APIC_DESTSH_SELF, 0x41);

	print_lapic_state();
	print_ioapic_states();

	print( is_kheap_corrupted() ? "KERNEL HEAP CORRUPTED" : "KERNEL HEAP OK" );
	print("\n");

	print("seconds passed from pit init: ");
	uint64_t ms = get_ms_from_tick_count(&pit, get_PIT_tick_count(&pit));
	print_uint64_dec(ms/1000);
	print(".");
	print_uint64_dec(ms%1000);
	print("\n");

	finalize_kio();
	return 0;
}
