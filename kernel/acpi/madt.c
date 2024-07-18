#include "madt.h"

static MADT* cached_madt = NULL;

MADT* get_MADT(){
	if(cached_madt != NULL)
		return cached_madt;

	ACPI_description_header* MADT_header = get_table_header(ACPI_SIGNATURE("APIC"));
	if(MADT_header == NULL)
		return NULL;
	if(!check_ACPI_table(MADT_header))
		return NULL;

	cached_madt = (MADT*) MADT_header;
	return cached_madt;
}

ICS_header* get_first_ICS_header(const MADT* madt){
	void* ptr = (void*)madt;
	ICS_header* header = (ICS_header*)(ptr+sizeof(MADT));
	uint64_t distance = (void*)(header)-(void*)(madt);
	if(distance >= madt->header.lenght)
		return NULL;
	return header;
}

ICS_header* get_next_ICS_header(const MADT* madt, const ICS_header* header){
	ICS_header* next = (void*)(header) + header->lenght;
	uint64_t distance = (void*)(next)-(void*)(madt);
	if(distance >= madt->header.lenght)
		return NULL;
	return next;
}

uint64_t count_number_of_local_apic(const MADT* madt){
	uint64_t count = 0;
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_PROCESSOR_LOCAL_APIC)
			count++;
	}
	return count;
}
