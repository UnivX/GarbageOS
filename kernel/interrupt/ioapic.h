#pragma once
#include "../acpi/madt.h"

typedef struct IOAPIC{
	ICS_IO_APIC* ics_ioapic;
	void* base_address;
} IOAPIC;

typedef struct IOAPICSubsystemData {
	IOAPIC* ioapic_array;
	uint64_t ioapic_array_size;

	ICS_interrupt_source_override** source_override_array;//array of ICS_interrupt_source_override pointers
	uint64_t source_override_array_size;
} IOAPICSubsystemData;

bool init_ioapic();
