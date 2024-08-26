#pragma once
#include "../hal.h"
#include "../kdefs.h"
#include "pic.h"

//these vector numbers are reserved for the pic use:
// 32-48 / 0x20-0x30
#define VMMCACHE_SHOOTDOWN_VECTOR 0x31
#define PIT_VECTOR 0x41

//these vector numbers are reserved for apic use
#define APIC_ERROR_VECTOR 0xf0
#define APIC_SPURIOUS_INTERRUPTS_VECTOR 0xff
//each interrupt_handler may have a void* called extra data
//this is a pointer to some specific data that may be used by the handler

typedef struct InterruptInfo{
	uint64_t error;
	uint64_t interrupt_number;
	void* extra_data;
	void* saved_context;
} InterruptInfo;

typedef void (InterruptHandler)(InterruptInfo);

void init_interrupts();
void init_interrupt_controller_subsystems();
void init_global_interrupt_controllers();
void init_local_interrupt_controllers();
void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler);
void set_interrupt_handler_extra_data(uint64_t interrupt_number, void* extra_data);
void install_default_interrupt_handler(InterruptHandler *handler);
