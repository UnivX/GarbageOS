#include "ioapic.h"
#include "../mem/heap.h"

static IOAPICSubsystemData ioapic_gdata = {NULL, 0, NULL, 0};

void fill_ioapic_array(const MADT* madt){
	KASSERT(madt != NULL);
	KASSERT(ioapic_gdata.ioapic_array != NULL);

	uint64_t ioapic_array_index = 0;
	//fill the APIC_ID and ics_lapic
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_PROCESSOR_IO_APIC){
			ICS_IO_APIC* ics_ioapic = (ICS_IO_APIC*)header;
			KASSERT(ioapic_array_index < ioapic_gdata.ioapic_array_size);

			ioapic_gdata.ioapic_array[ioapic_array_index].ics_ioapic = ics_ioapic;
			ioapic_gdata.ioapic_array[ioapic_array_index].base_address = (void*)((uint64_t)ics_ioapic->IO_APIC_address);
			ioapic_array_index++;
		}
	}
	KASSERT(ioapic_array_index == ioapic_gdata.ioapic_array_size);
}


void fill_source_override_array(const MADT* madt){
	KASSERT(madt != NULL);
	if(ioapic_gdata.source_override_array_size == 0)
		return;

	KASSERT(ioapic_gdata.source_override_array != NULL);


	uint64_t soverride_array_index = 0;
	//fill the APIC_ID and ics_lapic
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_INTERRUPT_SOURCE_OVERRIDE){
			ICS_interrupt_source_override* ics_iso = (ICS_interrupt_source_override*)header;
			KASSERT(soverride_array_index < ioapic_gdata.source_override_array_size);

			ioapic_gdata.source_override_array[soverride_array_index] = ics_iso;
			soverride_array_index++;
		}
	}
	KASSERT(soverride_array_index == ioapic_gdata.source_override_array_size);
}

bool init_ioapic(){
	MADT* madt = get_MADT();
	if(madt == NULL)
		return false;

	uint64_t count_of_ioapic = count_ICS_header_instances(madt, ICS_TYPE_PROCESSOR_IO_APIC);
	ioapic_gdata.ioapic_array = kmalloc(count_of_ioapic * sizeof(IOAPIC));
	ioapic_gdata.ioapic_array_size = count_of_ioapic;

	uint64_t count_of_source_overrides = count_ICS_header_instances(madt, ICS_TYPE_INTERRUPT_SOURCE_OVERRIDE);
	if(count_of_source_overrides != 0)
		ioapic_gdata.source_override_array = kmalloc(count_of_source_overrides * sizeof(ICS_interrupt_source_override*));
	ioapic_gdata.source_override_array_size = count_of_source_overrides;


	fill_ioapic_array(madt);


	return true;
}
