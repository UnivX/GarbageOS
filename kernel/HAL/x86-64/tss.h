#pragma once
#include <stdint.h>
#include "../../mem/memory.h"
#include "../../kdefs.h"
#include "../../hal.h"


typedef struct TSS{
	uint32_t reserved0;
	void* rsp0;
	void* rsp1;
	void* rsp2;
	uint64_t reserved1;
	void* ist1;
	void* ist2;
	void* ist3;
	void* ist4;
	void* ist5;
	void* ist6;
	void* ist7;
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb;
}__attribute__((packed)) TSS;

void init_tss(TSS* tss);
