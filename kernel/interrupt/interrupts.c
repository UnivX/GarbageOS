#include "interrupts.h"
#include "../kio.h"
#include "apic.h"
#define MAX_INTERRUPTS 256

//array of function pointers
InterruptHandler* handlers[MAX_INTERRUPTS];
InterruptHandler* default_handler;

void init_interrupts(){
	init_pic();
	disable_pic();
	default_handler = NULL;
	for(int i = 0; i < MAX_INTERRUPTS; i++)
		handlers[i] = NULL;
}

void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler){
	KASSERT(interrupt_number < MAX_INTERRUPTS);
	handlers[interrupt_number] = handler;
}

void install_default_interrupt_handler(InterruptHandler *handler){
	default_handler = handler;
}

//this function will be called by some assembly defined in the HAL
void interrupt_routine(uint64_t interrupt_number, uint64_t error){
	KASSERT(interrupt_number < MAX_INTERRUPTS);

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

	if(interrupt_number == APIC_SPURIOUS_INTERRUPTS_VECTOR){
#ifdef PRINT_ALL_SPURIOUS_INTERRUPT
		print("APIC spurious interrupt\n");
#endif
		//if it's a spurious interrupt then return
		return;
	}

	InterruptInfo info = {error, interrupt_number};

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
			send_lapic_EOI();
#endif
		}
	}
	return;
}
