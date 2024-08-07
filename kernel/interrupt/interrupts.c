#include "interrupts.h"
#include "../kio.h"
#include "apic.h"
#include "ioapic.h"
#include "../acpi/madt.h"
#define MAX_INTERRUPTS 256

bool pic_present = false;

//TODO: protect internal state from race conditions
//array of function pointers
static InterruptHandler* handlers[MAX_INTERRUPTS];
static void* extra_data_array[MAX_INTERRUPTS];
static InterruptHandler* default_handler;

void init_interrupt_controllers(){
	MADT* madt = get_MADT();
	if(madt == NULL)
		kpanic(GENERIC_ERROR);

	if(madt_has_pic(madt)){
		pic_present = true;
		init_pic();
		disable_pic();
	}

	KASSERT(init_apic());
	KASSERT(init_ioapics());
}

void init_interrupts(){
	default_handler = NULL;
	for(int i = 0; i < MAX_INTERRUPTS; i++){
		handlers[i] = NULL;
		extra_data_array[i] = NULL;
	}
}

void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler){
	KASSERT(interrupt_number < MAX_INTERRUPTS);
	handlers[interrupt_number] = handler;
}

void install_default_interrupt_handler(InterruptHandler *handler){
	default_handler = handler;
}

void set_interrupt_handler_extra_data(uint64_t interrupt_number, void* extra_data){
	KASSERT(interrupt_number < MAX_INTERRUPTS);
	extra_data_array[interrupt_number] = extra_data;
}

//this function will be called by some assembly defined in the HAL
void interrupt_routine(uint64_t interrupt_number, uint64_t error){
	KASSERT(interrupt_number < MAX_INTERRUPTS);

	if(pic_present){
		//we do not use the PIC but we may have to deal with PIC spurious interrupts
		if(interrupt_number-PIC_IDT_START < PIC_IDT_SIZE){
			bool is_spurious = is_pic_interrupt_spurious(interrupt_number);
#ifdef PRINT_ALL_SPURIOUS_INTERRUPT
			if(is_spurious)
				print("PIC spurious interrupt\n");
#endif
			pic_ack_interrupt(interrupt_number, is_spurious);
			return;
		}
	}

	if(interrupt_number == APIC_SPURIOUS_INTERRUPTS_VECTOR){
#ifdef PRINT_ALL_SPURIOUS_INTERRUPT
		print("APIC spurious interrupt\n");
#endif
		//if it's a spurious interrupt then return
		return;
	}

	InterruptInfo info = {error, interrupt_number, extra_data_array[interrupt_number]};

	if(handlers[interrupt_number] != NULL)
		handlers[interrupt_number](info);
	else if (default_handler != NULL)
		default_handler(info);

	if(is_local_apic_initialized()){
		if(is_interrupt_lapic_generated(interrupt_number)){
#ifdef PRINT_ALL_EOI

			print("sending local apic(id=");
			print_uint64_dec(get_logical_core_lapic_id());
			print(") EOI\n");
#endif
			send_lapic_EOI();
		}
	}
	return;
}
