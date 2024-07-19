#include "apic.h"
//TODO: check if it works

static LAPICSubsystemData lapic_gdata = {NULL, 0, NULL};

LocalAPIC* get_lapic_from_id(uint32_t apic_id){
	KASSERT(lapic_gdata.lapic_array != NULL);
	for(uint64_t i = 0; i < lapic_gdata.lapic_array_size; i++){
		if(lapic_gdata.lapic_array[i].APIC_ID == apic_id)
			return &(lapic_gdata.lapic_array[i]);
	}
	return NULL;
}

LocalAPIC* get_lapic_from_processor_uid(uint32_t processor_uid){
	KASSERT(lapic_gdata.lapic_array != NULL);
	for(uint64_t i = 0; i < lapic_gdata.lapic_array_size; i++){
		KASSERT(lapic_gdata.lapic_array[i].ics_lapic != NULL);
		if(lapic_gdata.lapic_array[i].ics_lapic->ACPI_processor_UID == processor_uid)
			return &(lapic_gdata.lapic_array[i]);
	}
	return NULL;
}

//TODO: split function
void fill_apic_array(MADT* madt){
	KASSERT(madt != NULL);

	uint64_t lapic_array_index = 0;
	//fill the APIC_ID and ics_lapic
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_PROCESSOR_LOCAL_APIC){
			ICS_local_APIC* ics_lapic = (ICS_local_APIC*)header;
			KASSERT(lapic_array_index < lapic_gdata.lapic_array_size);

			lapic_gdata.lapic_array[lapic_array_index].APIC_ID = ics_lapic->APIC_ID;
			lapic_gdata.lapic_array[lapic_array_index].ics_lapic = ics_lapic;
			lapic_array_index++;
		}
	}
	KASSERT(lapic_array_index == lapic_gdata.lapic_array_size);
	
	//fill ics_nmi
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_LOCAL_APIC_NMI){
			ICS_local_APIC_NMI* ics_lapic_nmi = (ICS_local_APIC_NMI*)header;
			
			//if the value is 0xff this option applyes for all the apics(read the ACPI specification on the MADT)
			if(ics_lapic_nmi->ACPI_processor_UID == 0xff){
				for(uint64_t i = 0; i < lapic_gdata.lapic_array_size; i++)
					lapic_gdata.lapic_array[i].ics_lapic_nmi = ics_lapic_nmi;
				break;//no need to continue the loop
			}else{
				LocalAPIC* lapic = get_lapic_from_processor_uid(ics_lapic_nmi->ACPI_processor_UID);
				KASSERT(lapic != NULL);
				lapic->ics_lapic_nmi = ics_lapic_nmi;
			}
		}
	}
}

void* get_lapic_base_address_from_madt(MADT* madt){
	uint64_t base_address = (uint64_t)madt->local_interrupt_controller_address;

	//search for an ICS_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE in the madt
	//and if present override the base address of the lapic
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE){
			ICS_local_APIC_address_override* ics_lapic_addr_override = (ICS_local_APIC_address_override*)header;
			base_address = ics_lapic_addr_override->local_APIC_address;//set the new 64bit address
		}
	}

	return (void*)base_address;
}

bool init_apic(){
	MADT* madt = get_MADT();
	if(madt == NULL)
		return false;

	uint64_t lapic_count = count_number_of_local_apic(madt);
	lapic_gdata.lapic_base_address = get_lapic_base_address_from_madt(madt);
	lapic_gdata.lapic_array_size = lapic_count;
	lapic_gdata.lapic_array = kmalloc(sizeof(LocalAPIC)*lapic_gdata.lapic_array_size);
	memset(lapic_gdata.lapic_array, 0, sizeof(LocalAPIC)*lapic_gdata.lapic_array_size);//set all fields to NULL/0
	
	fill_apic_array(madt);

	return true;
}
