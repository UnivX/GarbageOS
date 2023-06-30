#include "../../hal.h"
#include "x64-memory.h"
#include "../../frame_allocator.h"
#include "gdt.h"
#include "tss.h"

static bool is_initialized = false;

void set_up_caching(){
	asm volatile(
			"mov %%cr0, %%rax\n"
			"and $~(1<<29), %%rax\n"
			"and $~(1<<30), %%rax\n"
			"mov %%rax, %%cr0\n"
			: : : "cc", "rax");
}

void set_up_gdt_tss(){
	const uint64_t gdt_size = 6;

	//alloc a page for the gdt and tss
	KASSERT(sizeof(TSS) + sizeof(GDT)*gdt_size < PAGE_SIZE);
	void* new_page = alloc_frame();//we will use the physical address
	GDT* gdt = (GDT*)new_page;
	TSS* tss = (TSS*)(new_page + sizeof(GDT)*gdt_size);

	//null descriptor
	gdt[0] = make_gdt(0,0,0,0);
	//kernel code descriptor
	gdt[1] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(0) | NOT_SYSTEM | EXECUTABLE | CONFORMING | READ_WRITE);
	//kernel data descriptor
	gdt[2] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(0) | NOT_SYSTEM | READ_WRITE);

	//user code descriptor
	gdt[3] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(3) | NOT_SYSTEM | EXECUTABLE | READ_WRITE);
	//user data descriptor
	gdt[4] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(3) | NOT_SYSTEM | READ_WRITE);

	//TSS
	init_tss(tss);
	tss->ist1 = alloc_stack(INTERRUPT_STACK_SIZE);
	tss->rsp0 = alloc_stack(SYSTEM_CALL_STACK_SIZE);
	gdt[5] = make_gdt((uint64_t)tss, sizeof(TSS)-1, LONG_MODE_FLAG,
			PRESENT | DPL(0) | TYPE_TSS_AVAIBLE);

	memory_fence();//force the flush of the cpu write buffer and Write Combining buffer
	load_gdt(gdt, gdt_size);
	asm volatile(
			"mov $0x50, %%ax\n"
			"ltr %%ax\n"
			: : : "cc", "ax");//load the tss descriptor
}

void init_cpu(){
	set_up_caching();
	set_up_gdt_tss();
}

void set_up_arch_layer(){
	identity_map_memory();
	init_cpu();
	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
