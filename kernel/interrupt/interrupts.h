#pragma once
#include "../hal.h"
#include "../kdefs.h"
#include "pic.h"

//these vector numbers are reserved for the pic use:
// 32-48 / 0x20-0x30

//these vector numbers are reserved for apic use
#define APIC_ERROR_VECTOR 0xf0
#define APIC_SPURIOUS_INTERRUPTS_VECTOR 0xff

typedef struct InterruptInfo{
	uint64_t error;
	uint64_t interrupt_number;
} InterruptInfo;

typedef void (InterruptHandler)(InterruptInfo);

void init_interrupts();
void init_interrupt_controllers();
void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler);
void install_default_interrupt_handler(InterruptHandler *handler);
