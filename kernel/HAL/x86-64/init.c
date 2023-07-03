#include "../../hal.h"
#include "x64-memory.h"
#include "../../frame_allocator.h"
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

static bool is_initialized = false;
extern uint64_t isr_stub_table[];
extern uint64_t isr_stub_table_end[];
extern void load_idt(IDTR*);

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

	//alloc a page for the gdt, gdtr and tss
	KASSERT(sizeof(GDTR) + sizeof(TSS) + sizeof(GDT)*gdt_size < PAGE_SIZE);
	void* new_page = alloc_frame();//we will use the physical address
	GDT* gdt = (GDT*)new_page;
	TSS* tss = (TSS*)(new_page + sizeof(GDT)*gdt_size);
	GDTR* gdtr = (GDTR*)(new_page + sizeof(TSS)+ sizeof(GDT)*gdt_size);

	//null descriptor
	gdt[0] = make_gdt(0,0,0,0);
	//kernel code descriptor
	gdt[1] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_CONFORMING | GDT_READ_WRITE);
	//kernel data descriptor
	gdt[2] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_READ_WRITE);

	//user code descriptor
	gdt[3] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_READ_WRITE);
	//user data descriptor
	gdt[4] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_READ_WRITE);

	//TSS
	init_tss(tss);
	tss->ist1 = alloc_stack(INTERRUPT_STACK_SIZE);
	tss->rsp0 = alloc_stack(SYSTEM_CALL_STACK_SIZE);
	gdt[5] = make_gdt((uint64_t)tss, sizeof(TSS)-1, GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_TYPE_TSS_AVAIBLE);

	memory_fence();//force the flush of the cpu write buffer and Write Combining buffer
	load_gdt(gdt, gdt_size, gdtr);
	asm volatile(
			"mov $0x50, %%ax\n"
			"ltr %%ax\n"
			: : : "cc", "ax");//load the tss descriptor
}

void set_up_idt(){
	KASSERT(isr_stub_table_end-isr_stub_table == INTERRUPT_NUMBER);

	//alloc a page for the IDT and IDTR
	KASSERT(sizeof(IDTEntry)*INTERRUPT_NUMBER <= PAGE_SIZE);
	void* new_page = alloc_frame();//we will use the physical address
	IDTEntry* idt_array = (IDTEntry*)new_page;
	IDTR* idtr = alloc_frame();//the idt_table is big one page

	for(int i = 0; i < INTERRUPT_NUMBER; i++){
		uint64_t isr = isr_stub_table[i];
		uint8_t ist1 = 1;
		uint8_t attributes_type = IDT_DPL(0) | IDT_PRESENT(1);
		if (i <= LAST_IDT_EXCEPTION)
			attributes_type |= IDT_TRAP_TYPE;
		else
			attributes_type |= IDT_INTERRUPT_TYPE;

		idt_array[i] = make_idt_entry((void*)isr, KERNEL_CODE_SELECTOR, ist1, attributes_type);
	}
	idtr->idt = (uint64_t)&idt_array[0];
	idtr->size = (sizeof(IDTEntry)*INTERRUPT_NUMBER) -1;
	memory_fence();
	load_idt(idtr);
}

void init_cpu(){
	set_up_caching();
	set_up_gdt_tss();
	set_up_idt();
}

void set_up_arch_layer(){
	identity_map_memory();
	init_cpu();
	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
