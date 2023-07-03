#include "interrupts.h"
#define MAX_INTERRUPTS 256

//array of function pointers
InterruptHandler* handlers[MAX_INTERRUPTS];

void init_interrupts(){
	init_pic();
	for(int i = 0; i < MAX_INTERRUPTS; i++)
		handlers[i] = NULL;
}

void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler){
	KASSERT(interrupt_number < MAX_INTERRUPTS);
	handlers[interrupt_number] = handler;
}

//this function will be called by some assembly defined in the HAL
void interrupt_routine(uint64_t interrupt_number, uint64_t error){
	KASSERT(interrupt_number < MAX_INTERRUPTS);

	//if the PIC has sent a spurious interrupt ignore it
	//if the interrupt number is not served by the pic this function
	//will handle that correctly
	bool is_spurious = is_interrupt_spurious(interrupt_number);

	if(handlers[interrupt_number] != NULL && !is_spurious)
		handlers[interrupt_number](error);

	//if the interrupt comes from the PIC
	if(interrupt_number-PIC_IDT_START < PIC_IDT_SIZE){
		ack_interrupt(interrupt_number, is_spurious);
	}
}
