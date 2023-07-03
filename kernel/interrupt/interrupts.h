#pragma once
#include "../hal.h"
#include "../kdefs.h"
#include "pic.h"

typedef void (InterruptHandler)(uint64_t error);

void init_interrupts();
void install_interrupt_handler(uint64_t interrupt_number, InterruptHandler *handler);
