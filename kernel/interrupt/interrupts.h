#pragma once
#include "../hal.h"
#include "../kdefs.h"
#include "pic.h"

#define APIC_ERROR_VECTOR 0xf0
#define APIC_SPURIOUS_INTERRUPTS_VECTOR 0xff

typedef struct InterruptInfo{
	uint64_t error;
	uint64_t interrupt_number;
} InterruptInfo;

typedef void (InterruptHandler)(InterruptInfo);

void init_interrupts();
void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler);
void install_default_interrupt_handler(InterruptHandler *handler);
