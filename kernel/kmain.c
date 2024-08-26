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
#include "mp/mp_initialization.h"

#include "test/kheap_test.h"
//first usable intel CPU in history for this kernel : XEON Nocona (Jun 2004)
//first usable AMD CPU in history for this kernel? : Opteron (Apr 2003).

//TODO: replace print with printf
//TODO: do tlb shutdown
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
	printf("[GENERAL PROTECTION FAULT] error: %h64\n", info.error);
	kpanic(UNRECOVERABLE_GPF);
}

void interrupt_print(InterruptInfo info){
	KASSERT(info.interrupt_number > 32);
	printf("[INT %u64] error: %h64\n", (uint64_t)info.interrupt_number, (uint64_t)info.error);
}

void freeze_interrupt(InterruptInfo info){
	printf("freezing cpu\n");
	kio_flush();
	freeze_cpu();
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
	print("\nSleeping 2s\n");
	kio_flush();
	PIT_wait_ms(&pit, 2000);
	
	//for(int i = 0; i < 4000; i++)
		//io_wait();

#ifdef DO_TESTS
	print("\n-------------------STARTING TESTS-------------------\n");
	print("starting stack overflower\n");
	stack_overflower(70000);
	asm volatile("int $0x40");
	heap_stress_test();
#endif
	print("\n-------------------PRINTING MEMORY USAGE-------------------\n");

	uint64_t kernel_bootloader_overhead = get_total_usable_RAM_size()-(get_number_of_free_frames()*PAGE_SIZE);
	print("kernel + bootloader memory overhead: ");
	print_uint64_dec(kernel_bootloader_overhead / MB);
	print(" MiB\n");

	print("kernel paging memory overhead: ");
	print_uint64_dec(get_paging_mem_overhead((void*)get_kernel_VMM_paging_structure()) / MB);
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

	printf("number of logical cores: %u64\n", get_number_of_usable_logical_cores());

	printf( madt_has_pic(get_MADT()) ? "the PC has PIC\n" : "the PC doesnt have PIC\n");


	//print_lapic_state();
	//print_ioapic_states();

	printf( is_kheap_corrupted() ? "KERNEL HEAP CORRUPTED\n" : "KERNEL HEAP OK\n" );
	
	printf("starting other CPUs\n");
	PIT_wait_ms(&pit, 2000);
	MPInitError mp_err = init_APs(&pit);
	if(mp_err == ERROR_MP_OK){
		printf("APs init OK(number of total CPUs: %u64", get_number_of_usable_logical_cores());
	}else{
		printf("APs initialization failed\n");
		kpanic(GENERIC_ERROR);
	}

	
	printf("sending interrupt %u64 to all cpus \n", 0x51);
	send_IPI_by_destination_shorthand(APIC_DESTSH_ALL_INCLUDING_SELF, 0x51);
	while(!is_IPI_sending_complete()) ;
	kio_flush();

	PIT_wait_ms(&pit, 2000);

	printf("allocating kernel vmem\n");
	VMemHandle thandle =allocate_kernel_virtual_memory(PAGE_SIZE*64, VM_TYPE_GENERAL_USE, 4*PAGE_SIZE, 4*PAGE_SIZE);
	printf("deallocating kernel vmem\n");
	deallocate_kernel_virtual_memory(thandle);
	printf("kernel vmem test done\n");
	kio_flush();

	PIT_wait_ms(&pit, 2000);

	printf("freezing other cpus\n");
	install_interrupt_handler(0xf0, freeze_interrupt);
	send_IPI_by_destination_shorthand(APIC_DESTSH_ALL_EXCLUDING_SELF, 0xf0);
	while(!is_IPI_sending_complete()) ;
	PIT_wait_ms(&pit, 100);

	PIT_wait_ms(&pit, 2000);
	uint64_t ms = get_ms_from_tick_count(&pit, get_PIT_tick_count(&pit));
	printf("seconds passed from pit init: %u64.%u64\n\n\n", ms/1000, ms%1000);

	finalize_kio();
	return 0;
}
