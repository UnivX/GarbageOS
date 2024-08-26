#include "ioapic.h"
#include "apic.h"
#include "../mem/heap.h"
#include "../kio.h"
#include "../kernel_data.h"

static IOAPICSubsystemData ioapic_gdata = {NULL, 0, NULL, 0};
static spinlock ioapic_lock;
static bool init_single_ioapic(IOAPIC *ioapic);

static void write_32_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint32_t value);
static uint32_t read_32_ioapic_register(const IOAPIC *ioapic, const uint8_t offset);

static void write_64_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint64_t value);
static uint64_t read_64_ioapic_register(const IOAPIC *ioapic, const uint8_t offset);

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
			KASSERT(ics_iso->bus == 0);//the only bus admited by the ACPI standard is the ISA with a value of 0

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
	if(count_of_ioapic == 0)
		return false;

	init_spinlock(&ioapic_lock);
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
	KASSERT(ioapic != NULL);
	//memory map the ioapic register to an uncacheable page(4K)
	VirtualMemoryManager* kernel_vmm = get_kernel_VMM_from_kernel_data();
	ioapic->ioapic_mmap = memory_map(kernel_vmm, ioapic->physical_base_address, PAGE_SIZE, PAGE_WRITABLE | PAGE_CACHE_DISABLE);
	if(is_vmemhandle_invalid(ioapic->ioapic_mmap))
		return false;
	ioapic->ioapic_id = (read_32_ioapic_register(ioapic, IOAPICID_OFFSET) >> 24) & 0x0f;

	uint32_t version_reg_data = read_32_ioapic_register(ioapic, IOAPICVER_OFFSET);
	ioapic->version = version_reg_data & 0xff;
	ioapic->maximum_redirection_entry = ((version_reg_data >> 16) & 0xff)+1;
	return true;
}

//apply the patch to the irq numbers as specified by the ICS_interrupt_source_override
ISA_irq_fix get_isa_fix(uint64_t irq){
	KASSERT(ioapic_gdata.source_override_array != NULL);
	ISA_irq_fix fix = {false, false, false, 0, 0, 0};//no fix needed value

	for(uint64_t i = 0; i < ioapic_gdata.source_override_array_size; i++){
		ICS_interrupt_source_override* ics_iso = ioapic_gdata.source_override_array[i];
		if(ics_iso->source == irq){
			print("ioapic isa fix!\n");
			fix.irq_override = true;
			fix.irq = ics_iso->global_system_interrupt;
			//if it's not bus conform
			uint8_t polarity_flag = ics_iso->flags & MPS_INTI_POLARITY_MASK;
			if(polarity_flag != MPS_INTI_POLARITY_BUS_CONFORM){
				KASSERT(polarity_flag != MPS_INTI_POLARITY_RESERVED);//check that the value is legal
				fix.polarity = polarity_flag ==  MPS_INTI_POLARITY_ACTIVE_LOW;
			}

			uint8_t trigger_mode_flag = ics_iso->flags & MPS_INTI_TRIGGER_MODE_MASK;
			if(trigger_mode_flag != MPS_INTI_TRIGGER_MODE_BUS_CONFORM){
				KASSERT(trigger_mode_flag != MPS_INTI_TRIGGER_MODE_RESERVED);//check that the value is legal
				fix.trigger_mode = trigger_mode_flag == MPS_INTI_TRIGGER_MODE_LEVEL_TRIGGERED;
			}
			return fix;
		}
	}

	return fix;//no fix needed
}

void setIOAPICInterruptRedirection(IOAPICInterruptRedirection redirection, uint64_t irq){
	ACQUIRE_SPINLOCK_HARD(&ioapic_lock);
	//we assume that the ISA irqs(0-15) will be mapped to the APIC's irqs 0-15
	//but we may need to fix that assumption as stated in the ACPI specification
	ISA_irq_fix isa_fix = get_isa_fix(irq);
	
	if(isa_fix.irq_override)
		irq = isa_fix.irq;

	if(isa_fix.trigger_override)
		redirection.trigger_mode = isa_fix.trigger_mode;

	if(isa_fix.polarity_override)
		redirection.polarity = isa_fix.polarity;

	uint64_t ioredirection_entry = 0;

	//set vectory
	ioredirection_entry |= redirection.vector;

	//set delivery mode
	if(redirection.reidirect_to_lowest_priority){
		ioredirection_entry |= APIC_DEL_MODE_LOWEST_PRIORITY << 8;
	}else{
		//check that the apic id is only 4 bit wide as specified in the ioapic specification
		KASSERT((redirection.apic_id_to_redirect & 0x0f) == redirection.apic_id_to_redirect);
		//set delivery mode and apic id
		ioredirection_entry |= APIC_DEL_MODE_FIXED << 8;
		ioredirection_entry |= (uint64_t)(redirection.apic_id_to_redirect) << 56;//set destination field
	}

	//the destination mode is already setted in physical mode by default(bit 11 = 0)
	ioredirection_entry |= redirection.polarity << 13;
	ioredirection_entry |= redirection.trigger_mode << 15;
	ioredirection_entry |= redirection.mask << 16;

	// get the ioapic for this irq
	IOAPIC* ioapic = NULL;

	for(uint64_t i = 0; i < ioapic_gdata.ioapic_array_size; i++){
		IOAPIC *ioa_itr = &ioapic_gdata.ioapic_array[i];
		uint64_t starting_irq = ioa_itr->ics_ioapic->global_system_interrupt_base;
		uint64_t ending_irq = starting_irq + ioa_itr->maximum_redirection_entry;

		if(irq >= starting_irq && irq < ending_irq){
			ioapic = ioa_itr;
			break;
		}
	}

	KASSERT(ioapic != NULL);
	//calculate the interrupt line
	uint64_t interrupt_line = irq - ioapic->ics_ioapic->global_system_interrupt_base;
	//caluculate the redirection table entry offset
	uint64_t redtbl_offset = IOREDTBL_OFFSET_START + (interrupt_line*2);
	write_64_ioapic_register(ioapic, redtbl_offset, ioredirection_entry);
	RELEASE_SPINLOCK_HARD(&ioapic_lock);
}

void print_ioapic_states(){
	if(ioapic_gdata.ioapic_array == NULL){
		print("NO IOAPIC DATA AVAILABLE\n");
		return;
	}
	ACQUIRE_SPINLOCK_HARD(&ioapic_lock);
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
	RELEASE_SPINLOCK_HARD(&ioapic_lock);
}

void* get_ioapic_virtual_base_address(const IOAPIC *ioapic){
	return get_vmem_addr(ioapic->ioapic_mmap);
}

void write_32_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint32_t value){
	//as the IOAPIC specification
	volatile uint8_t* ioregsel = (volatile uint8_t*)get_ioapic_virtual_base_address(ioapic);
	volatile uint32_t* ioregwin = (volatile uint32_t*)(get_ioapic_virtual_base_address(ioapic)+0x10);
	InterruptState is = disable_and_save_interrupts();
	*ioregsel = offset;
	*ioregwin = value;
	restore_interrupt_state(is);
}

uint32_t read_32_ioapic_register(const IOAPIC *ioapic, const uint8_t offset){
	//as the IOAPIC specification
	volatile uint8_t* ioregsel = (volatile uint8_t*)get_ioapic_virtual_base_address(ioapic);
	volatile uint32_t* ioregwin = (volatile uint32_t*)(get_ioapic_virtual_base_address(ioapic)+0x10);
	InterruptState is = disable_and_save_interrupts();
	*ioregsel = offset;
	uint32_t value = *ioregwin;
	restore_interrupt_state(is);
	return value;
}

static void write_64_ioapic_register(const IOAPIC *ioapic, const uint8_t offset, const uint64_t value){
	uint32_t low_value = value & 0x00000000ffffffff;
	uint32_t high_value = (value >> 32) & 0x00000000ffffffff;

	InterruptState is = disable_and_save_interrupts();

	write_32_ioapic_register(ioapic, offset, low_value);
	write_32_ioapic_register(ioapic, offset+1, high_value);

	restore_interrupt_state(is);
}

static uint64_t read_64_ioapic_register(const IOAPIC *ioapic, const uint8_t offset){
	uint64_t value = 0;

	InterruptState is = disable_and_save_interrupts();
	value |= read_32_ioapic_register(ioapic, offset);
	value |= (uint64_t)read_32_ioapic_register(ioapic, offset+1) << 32;
	restore_interrupt_state(is);

	return value;
}
