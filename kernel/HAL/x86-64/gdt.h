#pragma once
#include <stdint.h>
#include "../../hal.h"

typedef struct GDT{
	uint16_t limit;
	uint16_t base1;
	uint8_t base2;
	uint8_t access;
	uint8_t limit_flags;
	uint8_t base3;
	uint32_t base4;
	uint32_t reserved;
}__attribute__((packed)) GDT;

typedef struct GDTR{
	uint16_t size;
	uint64_t gdt;
}__attribute__((packed)) GDTR;

#define GDT_GRANULARITY_FLAG 1 << 3
#define GDT_SIZE_FLAG 1 << 2
#define GDT_LONG_MODE_FLAG 1 << 1

#define GDT_PRESENT 1 << 7
#define GDT_DPL(x) x << 5
#define GDT_NOT_SYSTEM 1 << 4
#define GDT_EXECUTABLE 1 << 3
#define GDT_DIRECTION 1 << 2
#define GDT_CONFORMING 1 << 2
#define GDT_READ_WRITE 1 << 1

#define GDT_TYPE_TSS_AVAIBLE 0x9
#define GDT_TYPE_TSS_BUSY 0xB
#define GDT_TYPE_LDT 0x2

GDT make_gdt(uint64_t base, uint32_t limit, uint8_t flags, uint8_t access);

//the gdtr pointer doesn't need to be initialized
void load_gdt(GDT* gdt, uint16_t size, GDTR* gdtr);
