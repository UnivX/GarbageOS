#include "apic.h"
#include "../kio.h"
#include <cpuid.h>
#include "interrupts.h"

//TODO: check if it works

static LAPICSubsystemData lapic_gdata = {NULL, 0, NULL, NULL};

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

bool apic_present(){
	unsigned int eax=0, unused=0, edx=0;
	__get_cpuid(1, &eax, &unused, &unused, &edx);
	return (edx & CPUID_APIC_PRESENCE_EDX) != 0;
}

bool is_apic_enabled(){
	if(!cpu_has_msr())
		return false;
	uint32_t low=0, high=0;
	get_cpu_msr(IA32_APIC_BASE_MSR, &low, &high);
	return (low & APIC_GLOBAL_ENABLE_DISABLE_FLAG) != 0;
}

bool init_apic(){
	if(!apic_present())
		return false;

	if(!is_apic_enabled())
		return false;

	MADT* madt = get_MADT();
	if(madt == NULL)
		return false;

	uint64_t lapic_count = count_number_of_local_apic(madt);
	lapic_gdata.lapic_base_address = get_lapic_base_address_from_madt(madt);
	lapic_gdata.lapic_array_size = lapic_count;
	lapic_gdata.lapic_array = kmalloc(sizeof(LocalAPIC)*lapic_gdata.lapic_array_size);
	memset(lapic_gdata.lapic_array, 0, sizeof(LocalAPIC)*lapic_gdata.lapic_array_size);//set all fields to NULL/0
	
	fill_apic_array(madt);

	KASSERT((uint64_t)lapic_gdata.lapic_base_address % PAGE_SIZE == 0);
	lapic_gdata.register_space_mapping = memory_map(lapic_gdata.lapic_base_address, APIC_REGISTER_SPACE_SIZE, PAGE_WRITABLE | PAGE_CACHE_DISABLE);

	return true;
}

void* get_register_space_start_addr(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	return get_vmem_addr(lapic_gdata.register_space_mapping);
}

void write_32_lapic_register(uint64_t register_offset, uint32_t value){
	*((uint32_t*)(get_register_space_start_addr() + register_offset)) = value;
}

uint32_t read_32_lapic_register(uint64_t register_offset){
	return *((uint32_t*)(get_register_space_start_addr() + register_offset));
}

uint32_t get_logical_core_lapic_id(){
	uint32_t raw_lapic_id = read_32_lapic_register(APIC_REG_OFFSET_LAPIC_ID);
	//xapic has the apic id from 24 to 31
	return (raw_lapic_id >> 24) & 0x000000ff;
}

bool can_suppress_EOI_broadcast(){
	return (read_32_lapic_register(APIC_REG_OFFSET_LAPIC_VERSION) & SUPPRESS_EOI_BROADCAST_VERSION_FLAG) != 0;
}


void read_apic_error(){
	
}

void init_current_logical_core_lapic(){
}


void print_lapic_state(){
	print("LOCAL APIC REGISTERS:\n");
	print("ID = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LAPIC_ID));
	print("\n");

	print("VERSION = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LAPIC_VERSION));
	print("\n");

	print("ERROR STATUS = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_ERROR_STATUS));
	print("\n");

	print("LVT CMCI = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_CMCI));
	print("\n");

	print("LVT TIMER = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_TIMER));
	print("\n");

	print("LVT THERMAL SENSOR = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_THERMAL_SENSOR));
	print("\n");

	print("LVT PERFORMANCE MONITORING COUNTERS = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_PERFORMANCE_MONITORING_COUNTERS));
	print("\n");

	print("LVT LINT0 = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_LINT0));
	print("\n");

	print("LVT LINT1 = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_LINT1));
	print("\n");

	print("LVT ERROR = ");
	print_uint64_hex(read_32_lapic_register(APIC_REG_OFFSET_LVT_ERROR));
	print("\n");
}
