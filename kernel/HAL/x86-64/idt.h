#pragma once
#include "../../kdefs.h"
#include "../../hal.h"
#include <stdint.h>

typedef struct IDTEntry{
	uint16_t offset1;
	uint16_t segment_selector;
	uint8_t ist;
	uint8_t attributes_type;
	uint16_t offset2;
	uint32_t offset3;
	uint32_t reserved;
}__attribute__((packed)) IDTEntry;

typedef struct IDTR{
	uint16_t size;
	uint64_t idt;
}__attribute__((packed)) IDTR;

#define IDT_INTERRUPT_TYPE 0xE
#define IDT_TRAP_TYPE 0xF
#define IDT_PRESENT(x) x << 7
#define IDT_DPL(x) x << 5

IDTEntry make_idt_entry(void* offset, uint16_t segment_selector, uint8_t ist, uint8_t attributes_type);
