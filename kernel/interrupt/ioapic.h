#pragma once
#include "../mem/vmm.h"
#include "../acpi/madt.h"

/*
00h IOAPICID IOAPIC ID R/W
01h IOAPICVER IOAPIC Version RO
02h IOAPICARB IOAPIC Arbitration ID RO
10−3Fh IOREDTBL[0:23] Redirection Table (Entries 0-23) (64 bits each) R/W
 */
#define IOAPICID_OFFSET 0x0
#define IOAPICVER_OFFSET 0x1
#define IOAPICARB_OFFSET 0x2
#define IOREDTBL_OFFSET_START 0x10

typedef struct IOAPIC{
	ICS_IO_APIC* ics_ioapic;
	void* physical_base_address;
	VMemHandle ioapic_mmap;
	
	//all these variables are read from the ioapic register
	uint8_t ioapic_id;
	uint8_t version;
	uint8_t maximum_redirection_entry;
} IOAPIC;

typedef struct IOAPICSubsystemData {
	IOAPIC* ioapic_array;
	uint64_t ioapic_array_size;

	ICS_interrupt_source_override** source_override_array;//array of ICS_interrupt_source_override pointers
	uint64_t source_override_array_size;
} IOAPICSubsystemData;

bool init_ioapics();
void print_ioapic_states();
