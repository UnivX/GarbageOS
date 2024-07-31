#include "ioapic.h"
#include "../mem/heap.h"
#include "../kio.h"

static IOAPICSubsystemData ioapic_gdata = {NULL, 0, NULL, 0};
static bool init_single_ioapic(IOAPIC *ioapic);
static void write_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint32_t value);
static uint32_t read_ioapic_register(const IOAPIC *ioapic, const uint8_t offset);

void fill_ioapic_array(const MADT* madt){
	KASSERT(madt != NULL);
	KASSERT(ioapic_gdata.ioapic_array != NULL);

	uint64_t ioapic_array_index = 0;
	//fill the APIC_ID and ics_lapic
	for(ICS_header* header = get_first_ICS_header(madt); header != NULL; header = get_next_ICS_header(madt, header)){
		if(header->type == ICS_TYPE_PROCESSOR_IO_APIC){
			ICS_IO_APIC* ics_ioapic = (ICS_IO_APIC*)header;
			KASSERT(ioapic_array_index < ioapic_gdata.ioapic_array_size);

			//set all the ioapic fields to null
			memset(&(ioapic_gdata.ioapic_array[ioapic_array_index]), 0, sizeof(IOAPIC));

			ioapic_gdata.ioapic_array[ioapic_array_index].ics_ioapic = ics_ioapic;
			ioapic_gdata.ioapic_array[ioapic_array_index].physical_base_address = (void*)((uint64_t)ics_ioapic->IO_APIC_address);
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

bool init_ioapics(){
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
	fill_source_override_array(madt);

	for(uint64_t i = 0; i < ioapic_gdata.ioapic_array_size; i++)
		if(!init_single_ioapic(&(ioapic_gdata.ioapic_array[i])))
			return false;
	
	return true;
}

bool init_single_ioapic(IOAPIC *ioapic){
	//memory map the ioapic register to an uncacheable page(4K)
	ioapic->ioapic_mmap = memory_map(ioapic->physical_base_address, PAGE_SIZE, PAGE_WRITABLE | PAGE_CACHE_DISABLE);
	if(ioapic->ioapic_mmap == NULL)
		return false;
	ioapic->ioapic_id = (read_ioapic_register(ioapic, IOAPICID_OFFSET) >> 24) & 0x0f;

	uint32_t version_reg_data = read_ioapic_register(ioapic, IOAPICVER_OFFSET);
	ioapic->version = version_reg_data & 0xff;
	ioapic->maximum_redirection_entry = ((version_reg_data >> 16) & 0xff)+1;
	return true;
}

void print_ioapic_states(){
	if(ioapic_gdata.ioapic_array == NULL){
		print("NO IOAPIC DATA AVAILABLE\n");
		return;
	}

	print("Number of ioapics: ");
	print_uint64_dec(ioapic_gdata.ioapic_array_size);
	print("\n");

	print("IOAPICS states: \n");
	for(uint64_t i = 0; i < ioapic_gdata.ioapic_array_size; i++){
		const IOAPIC* ioapic = &(ioapic_gdata.ioapic_array[i]);
		print("---------------------------------\n");

		print("IOAPIC madt ID: ");
		print_uint64_dec(ioapic->ics_ioapic->IO_APIC_ID);
		print("\n");

		print("IOAPIC read ID: ");
		print_uint64_dec(ioapic->ioapic_id);
		print("\n");

		print("IOAPIC physical address: ");
		print_uint64_hex((uint64_t)ioapic->physical_base_address);
		print("\n");

		print("IOAPIC version: ");
		print_uint64_dec(ioapic->version);
		print("\n");

		print("IOAPIC maximum redirection entry: ");
		print_uint64_dec(ioapic->maximum_redirection_entry);
		print("\n");
	}
}

void* get_ioapic_virtual_base_address(const IOAPIC *ioapic){
	return get_vmem_addr(ioapic->ioapic_mmap);
}

void write_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint32_t value){
	//as the IOAPIC specification
	volatile uint8_t* ioregsel = (volatile uint8_t*)get_ioapic_virtual_base_address(ioapic);
	volatile uint32_t* ioregwin = (volatile uint32_t*)(get_ioapic_virtual_base_address(ioapic)+0x10);
	*ioregsel = offset;
	*ioregwin = value;
}

uint32_t read_ioapic_register(const IOAPIC *ioapic, const uint8_t offset){
	//as the IOAPIC specification
	volatile uint8_t* ioregsel = (volatile uint8_t*)get_ioapic_virtual_base_address(ioapic);
	volatile uint32_t* ioregwin = (volatile uint32_t*)(get_ioapic_virtual_base_address(ioapic)+0x10);
	*ioregsel = offset;
	uint32_t value = *ioregwin;
	return value;
}
