#pragma once
#include "../kdefs.h"
#include "../hal.h"
#include "../acpi/madt.h"
#include "../mem/heap.h"


//do not support X2APIC

typedef struct LocalAPIC{
	uint32_t APIC_ID;
	ICS_local_APIC* ics_lapic;
	ICS_local_APIC_NMI* ics_lapic_nmi;
} LocalAPIC;

typedef struct LAPICSubsystemData{
	LocalAPIC* lapic_array;
	uint64_t lapic_array_size;
	void* lapic_base_address;
} LAPICSubsystemData;

bool init_apic();
