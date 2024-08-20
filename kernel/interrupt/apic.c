#include "apic.h"
#include "../kio.h"
#include <cpuid.h>
#include "interrupts.h"
#include "../hal.h"
#include "../kdefs.h"
#include "../util/sync_types.h"

static LAPICSubsystemData lapic_gdata = {NULL, 0, NULL, NULL};

//check if it's the deiscrete apic chip(82489DX)
static bool is_lapic_discrete_82489DX();
static void apic_error_interrupt_handler(InterruptInfo info);
void send_lapic_EOI();

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

bool init_apic_subsystem(){
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
	lapic_gdata.register_space_mapping = memory_map((void*)lapic_gdata.lapic_base_address, APIC_REGISTER_SPACE_SIZE, PAGE_WRITABLE | PAGE_CACHE_DISABLE);

	install_interrupt_handler(APIC_ERROR_VECTOR, apic_error_interrupt_handler);

	return true;
}

bool is_local_apic_initialized(){
	if(lapic_gdata.register_space_mapping == NULL)
		return false;
	uint32_t lapic_id = get_logical_core_lapic_id();
	LocalAPIC* lapic_data = get_lapic_from_id(lapic_id);
	KASSERT(lapic_data != NULL);
	return lapic_data->enabled;
}

void* get_register_space_start_addr(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	return get_vmem_addr(lapic_gdata.register_space_mapping);
}

void write_32_lapic_register(uint64_t register_offset, uint32_t value){
	*((volatile uint32_t*)(get_register_space_start_addr() + register_offset)) = value;
}

uint32_t read_32_lapic_register(uint64_t register_offset){
	return *((volatile uint32_t*)(get_register_space_start_addr() + register_offset));
}

void write_64_lapic_register(uint64_t reg_low, uint64_t value){
	uint32_t low_value = value & 0x00000000ffffffff;
	uint32_t high_value = (value >> 32) & 0x00000000ffffffff;

	//make it ininterruptible so we do not have race condition
	//(we do not have race condition because the registers are r/w-able only from the current logical core)
	InterruptState interrupt_state = disable_and_save_interrupts();
	//critical section start
	*((volatile uint32_t*)(get_register_space_start_addr() + reg_low)) = low_value;
	*((volatile uint32_t*)(get_register_space_start_addr() + reg_low + 0x10)) = high_value;
	//critical section end
	restore_interrupt_state(interrupt_state);
}

uint64_t read_64_lapic_register(uint64_t reg_low){
	//make it ininterruptible so we do not have race condition
	//(we do not have race condition because the registers are r/w-able only from the current logical core)
	InterruptState interrupt_state = disable_and_save_interrupts();
	//critical section start
	uint64_t low = *((volatile uint32_t*)(get_register_space_start_addr() + reg_low));
	uint64_t high = *((volatile uint32_t*)(get_register_space_start_addr() + reg_low + 0x10));//offset to the next apic register
	//critical section end
	restore_interrupt_state(interrupt_state);
	return low | (high << 32);
}

uint32_t get_logical_core_lapic_id(){
	uint32_t raw_lapic_id = read_32_lapic_register(APIC_REG_OFFSET_LAPIC_ID);
	//xapic has the apic id from 24 to 31
	return (raw_lapic_id >> 24) & 0x000000ff;
}

bool can_suppress_EOI_broadcast(){
	return (read_32_lapic_register(APIC_REG_OFFSET_LAPIC_VERSION) & SUPPRESS_EOI_BROADCAST_VERSION_FLAG) != 0;
}


uint32_t read_apic_error(){
	write_32_lapic_register(APIC_REG_OFFSET_ERROR_STATUS, 0);
	return read_32_lapic_register(APIC_REG_OFFSET_ERROR_STATUS);
}

uint32_t make_lvt_entry(uint8_t vector, APICDeliveryMode del_mode, bool interrupt_pin_polarity,
		bool remote_irr_flag, bool trigger_mode, bool mask)
{
	uint32_t lvt = vector;
	uint8_t raw_delivery_mode = (uint8_t)del_mode & 0b111;
	lvt |= raw_delivery_mode << 8;
	lvt |= ((uint8_t)interrupt_pin_polarity) << 13;
	lvt |= ((uint8_t)remote_irr_flag) << 14;
	lvt |= ((uint8_t)trigger_mode) << 15;
	lvt |= ((uint8_t)mask) << 16;
	return lvt;
}

void setup_lapic_nmi_lint(){
	//get lapic data
	uint32_t lapic_id = get_logical_core_lapic_id();
	LocalAPIC* lapic_data = get_lapic_from_id(lapic_id);
	KASSERT(lapic_data != NULL);

	//check if there is a lint# to set as an nmi
	if(lapic_data->ics_lapic_nmi != NULL){
		//get the register offset of the lint# to set the nmi
		uint32_t lint_reg = 0;
		switch(lapic_data->ics_lapic_nmi->local_APIC_LINT){
			case 0:
				lint_reg = APIC_REG_OFFSET_LVT_LINT0;
				break;
			case 1:
				lint_reg = APIC_REG_OFFSET_LVT_LINT1;
				break;
			default:
				kpanic(APIC_ERROR);
				break;
		}
		KASSERT(lint_reg != 0);
		//getting the polarity and the trigger_mode from the flags of the ics_lapic_nmi
		uint8_t polarity_flag = lapic_data->ics_lapic_nmi->flags & MPS_INTI_POLARITY_MASK;
		uint8_t trigger_mode_flag = lapic_data->ics_lapic_nmi->flags & MPS_INTI_TRIGGER_MODE_MASK;

		//standard polarity and trigger mode values for the NMI
		bool polarity = 0;//active low
		bool trigger_mode = 0;//edge triggered
							  
		//if not bus conform read the value from the acpi flags
		if(polarity_flag != MPS_INTI_POLARITY_BUS_CONFORM)
			polarity = polarity_flag == MPS_INTI_POLARITY_ACTIVE_LOW;//1 = active low, 0 = active high

		if(polarity_flag != MPS_INTI_TRIGGER_MODE_BUS_CONFORM)
			trigger_mode = trigger_mode_flag == MPS_INTI_TRIGGER_MODE_LEVEL_TRIGGERED;//1 = level, 0 = edge

		uint32_t lint_lvt = make_lvt_entry(0xff, APIC_DEL_MODE_NMI, polarity, 0, trigger_mode, false);
		write_32_lapic_register(lint_reg, lint_lvt);
	}
}

bool init_local_apic(){
	//enable lapic
	const uint32_t apic_enable_flag = 0x100;
	write_32_lapic_register(APIC_REG_OFFSET_SPURIOUS_INTERRUPT_VECTOR, APIC_SPURIOUS_INTERRUPTS_VECTOR | apic_enable_flag);
	write_32_lapic_register(APIC_REG_OFFSET_TASK_PRIORITY, 0);


	//unmask the error lvt to enable the apic error interrupt
	uint32_t error_lvt = make_lvt_entry(APIC_ERROR_VECTOR, APIC_DEL_MODE_FIXED, 0, 0, 0, 0);
	write_32_lapic_register(APIC_REG_OFFSET_LVT_ERROR, error_lvt);

	//check if there is an error while enabling the error lvt
	if(read_apic_error() != 0)
		return false;

	setup_lapic_nmi_lint();

	uint32_t lapic_id = get_logical_core_lapic_id();
	LocalAPIC* lapic_data = get_lapic_from_id(lapic_id);
	KASSERT(lapic_data != NULL);
	lapic_data->enabled = true;
	

	//simulate error
	//write_32_lapic_register(APIC_REG_OFFSET_LVT_CMCI, make_lvt_entry(5, APIC_DEL_MODE_FIXED, 0, 0, 0, 1));
	return true;
}

//NOT FOR NMI, SMI, INIT, ExtInt, SIPI, only fixed interrupt
void send_lapic_EOI(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	write_32_lapic_register(APIC_REG_OFFSET_EOI, 0);
}

uint64_t make_interrupt_command(uint8_t destination_id, APICDestinationShorthand dest_sh, uint8_t trigger_mode,
		uint8_t level, uint8_t destination_mode, APICDeliveryMode del_mode, uint8_t vector)
{
	uint64_t ICR = (uint64_t)destination_id << 56;
	ICR |= (uint64_t)(dest_sh & 0b11) << 18;
	ICR |= (uint64_t)(trigger_mode & 1) << 15;
	ICR |= (uint64_t)(level & 1) << 14;
	ICR |= (uint64_t)(destination_mode & 1) << 11;
	ICR |= (uint64_t)(del_mode & 0b111) << 8;
	ICR |= vector;
	return ICR;
}

bool is_IPI_sending_complete(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	const uint64_t delivery_status_flag = (1<<12);
	return (read_64_lapic_register(APIC_REG_OFFSET_INTERRUPT_COMMAND_LOW) & delivery_status_flag) == 0;
}

//this function waits for the end of the sending of the IPI
void wait_and_write_interrupt_command_register(uint64_t ICR){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	bool write_done = false;
	while(!write_done){
		//make it ininterruptible so we do not have race condition
		//(we do not have race condition because the registers are r/w-able only from the current logical core)
		InterruptState is = disable_and_save_interrupts();
		if(is_IPI_sending_complete()){
			write_64_lapic_register(APIC_REG_OFFSET_INTERRUPT_COMMAND_LOW, ICR);
			write_done = true;
		}
		restore_interrupt_state(is);
	}
}

void send_IPI_by_destination_shorthand(APICDestinationShorthand dest_sh, uint8_t interrupt_vector){
	KASSERT(dest_sh != APIC_DESTSH_NO_SH);
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(0, dest_sh, 0, 1, 0, APIC_DEL_MODE_FIXED, interrupt_vector);
	wait_and_write_interrupt_command_register(ICR);
}

void send_IPI_by_lapic_id(uint32_t lapic_id_target, uint8_t interrupt_vector){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(lapic_id_target, APIC_DESTSH_NO_SH, 0, 1, 0, APIC_DEL_MODE_FIXED, interrupt_vector);
	wait_and_write_interrupt_command_register(ICR);
}

void send_IPI_INIT_by_lapic_id(uint32_t lapic_id_target){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(lapic_id_target, APIC_DESTSH_NO_SH, 0, 1, 0, APIC_DEL_MODE_INIT, 0);
	wait_and_write_interrupt_command_register(ICR);
}

void send_IPI_INIT_to_all_excluding_self(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(0, APIC_DESTSH_ALL_EXCLUDING_SELF, 0, 1, 0, APIC_DEL_MODE_INIT, 0);
	wait_and_write_interrupt_command_register(ICR);
}

void send_IPI_INIT_deassert(){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(0, APIC_DESTSH_ALL_INCLUDING_SELF, 1, 0, 0, APIC_DEL_MODE_INIT, 0);
	wait_and_write_interrupt_command_register(ICR);
}

void send_IPI_INIT_deassert_by_lapic_id(uint32_t apic_id){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(apic_id, APIC_DESTSH_NO_SH, 1, 0, 0, APIC_DEL_MODE_INIT, 0);
	wait_and_write_interrupt_command_register(ICR);
}

void send_startup_IPI(uint32_t lapic_id_target, uint8_t interrupt_vector){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(lapic_id_target, APIC_DESTSH_NO_SH, 0, 1, 0, APIC_DEL_MODE_START_UP, interrupt_vector);
	wait_and_write_interrupt_command_register(ICR);
}

void send_startup_IPI_to_all_excluding_self(uint8_t interrupt_vector){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	uint64_t ICR = make_interrupt_command(0, APIC_DESTSH_ALL_EXCLUDING_SELF, 0, 1, 0, APIC_DEL_MODE_START_UP, interrupt_vector);
	wait_and_write_interrupt_command_register(ICR);
}

bool is_interrupt_lapic_generated(uint8_t interrupt_vector){
	KASSERT(lapic_gdata.register_space_mapping != NULL);
	//the ISR is a 256 bit wide register
	//starting at APIC_REG_OFFSET_IN_SERVICE_FIRST
	//each dword is then at a register offseted by the previous by +0x10
	uint32_t register_number = interrupt_vector / 32;
	uint32_t register_value_bit_number = interrupt_vector % 32;
	uint32_t register_offset = APIC_REG_OFFSET_IN_SERVICE_FIRST + (register_number*0x10);

	uint32_t ISR_dword = read_32_lapic_register(register_offset);
	return (ISR_dword & (1 << register_value_bit_number)) != 0;
}

bool is_lapic_discrete_82489DX(){
	uint32_t version = read_32_lapic_register(APIC_REG_OFFSET_LAPIC_VERSION) & 0xff;
	return version < 0x10;
}

int64_t get_lapic_id_array(uint32_t* buffer, uint64_t buffer_size){
	if(buffer_size < lapic_gdata.lapic_array_size)
		return -1;
	for(uint64_t i = 0; i < lapic_gdata.lapic_array_size; i++){
		buffer[i] = lapic_gdata.lapic_array[i].APIC_ID;
	}
	return lapic_gdata.lapic_array_size;
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

void apic_error_interrupt_handler(InterruptInfo info){
	print("APIC ERROR: ");
	print_uint64_hex(read_apic_error());
	print("\n");
	kpanic(APIC_ERROR);
}
