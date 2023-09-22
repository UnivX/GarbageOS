#pragma once
#include <stdint.h>
#include "../../hal.h"
#include "x64-memory.h"
#include "../../mem/frame_allocator.h"
#include "../../mem/vmm.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"

#define LAST_IDT_EXCEPTION 31

#define KERNEL_CODE_SELECTOR 0x10
#define KERNEL_DATA_SELECTOR 0x20
#define USER_CODE_SELECTOR 0x30
#define USER_DATA_SELECTOR 0x40
#define TSS_SELECTOR 0x50

#define INTERRUPT_NUMBER 256

#define DOUBLE_FAULT_INT_NUMBER 0x8
#define NMI_INT_NUMBER 0x2

typedef struct LocalCPUState{
	TSS* tss;
} LocalCPUState;

typedef struct GlobalCPUState{
	GDT* gdt;
	GDTR* gdtr;
	IDTEntry* idt;
	IDTR* idtr;

	uint32_t number_of_logical_cores;
	
} GlobalCPUState;
