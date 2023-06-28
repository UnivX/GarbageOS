#include "gdt.h"
extern void _gdt_flush();

void* _global_gdtr;

GDT make_gdt(uint64_t base, uint32_t limit, uint8_t flags, uint8_t access){
	KASSERT(sizeof(GDT) == 16);
	GDT result;

	result.limit = (limit) & 0xffff; 
	result.base1 = (base & 0xffff);
	result.base2 = (base>>16) & 0xff;
	result.access = access;
	result.limit_flags = ((limit>>16) & 0xf) | ((flags << 4) & 0xf0);
	result.base3 = (base >> 24) & 0xff;
	result.base4 = (base >> 32) & 0xffffffff;
	return result;
}

GDTR gdtr;
void load_gdt(GDT* gdt, uint16_t size){
	uint64_t gdt_ptr = (uint64_t)gdt;
	gdtr.gdt = gdt_ptr;
	gdtr.size = size*sizeof(GDT)-1;
	_global_gdtr = &gdtr;
	_gdt_flush();
}
