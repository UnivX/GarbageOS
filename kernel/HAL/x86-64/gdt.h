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

#define GRANULARITY_FLAG 1 << 3
#define SIZE_FLAG 1 << 2
#define LONG_MODE_FLAG 1 << 1

#define PRESENT 1 << 7
#define DPL(x) x << 5
#define NOT_SYSTEM 1 << 4
#define EXECUTABLE 1 << 3
#define DIRECTION 1 << 2
#define CONFORMING 1 << 2
#define READ_WRITE 1 << 1

GDT make_gdt(uint64_t base, uint32_t limit, uint8_t flags, uint8_t access);

void load_gdt(GDT* gdt, uint16_t size);
