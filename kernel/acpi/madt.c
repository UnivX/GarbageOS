#include "madt.h"

MADT* get_MADT(){
	ACPI_description_header* MADT_header = get_table_header(ACPI_SIGNATURE("APIC"));
	if(MADT_header == NULL)
		return NULL;
	if(!check_ACPI_table(MADT_header))
		return NULL;
	return (MADT*) MADT_header;
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
