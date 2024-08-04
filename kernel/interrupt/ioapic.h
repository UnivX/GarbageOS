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

//only supported destination mode is the physical
typedef struct IOAPICInterruptRedirection{
	bool reidirect_to_lowest_priority;//if this flag is set the interrupt will be sent to the apic with the lowest priority register(AKA cr8)
	uint8_t apic_id_to_redirect;//it uses only the first 4 bit for a maximum of 16 addressable CPUs
	uint8_t trigger_mode;//1 = level sensitive, 0 = edge sensitive
	uint8_t polarity;//1 = low active, 0 = high active
	bool mask;//1 = masked(no interrupt will be received), 0 = not masked (interrupt will be received)
	uint8_t vector;//interrupt vector to use
	
} IOAPICInterruptRedirection;

typedef struct ISA_irq_fix{
	//these flags are true if the polarity or trigger have changed from the ISA standard and need to be overrided
	bool irq_override;
	bool polarity_override;
	bool trigger_override;

	uint64_t irq;//the new irq of the APIC if the override is needed
	uint8_t polarity;//polarity value if override is needed(same values as the IOAPICInterruptRedirection)
	uint8_t trigger_mode;//trigger_mode value if override is needed(same values as the IOAPICInterruptRedirection)
} ISA_irq_fix;

bool init_ioapics();
void print_ioapic_states();
void setIOAPICInterruptRedirection(IOAPICInterruptRedirection redirection, uint64_t irq);
