#include "idt.h"

IDTEntry make_idt_entry(void* offset, uint16_t segment_selector, uint8_t ist, uint8_t attributes_type){
	uint64_t ioffset = (uint64_t)offset;
	KASSERT(ist <= 7);//the ist must be a 3 bit value
	KASSERT((attributes_type & (1<<4)) == 0)

	IDTEntry entry;
	entry.segment_selector = segment_selector;
	entry.ist = ist;
	entry.attributes_type = attributes_type;
	entry.offset1 = ioffset;
	entry.offset2 = ioffset >> 16;
	entry.offset3 = ioffset >> 32;
	return entry;
}
