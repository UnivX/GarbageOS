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

GDT* global_gdt = NULL;
uint64_t number_of_tss = 1;

void set_up_gdt_tss(){
	const uint64_t gdt_size = 6;

	//alloc a page for the gdt, gdtr and tss
	KASSERT(sizeof(GDTR) + sizeof(TSS) <= PAGE_SIZE);
	KASSERT(sizeof(GDT)*gdt_size <= PAGE_SIZE);
	void* new_page = alloc_frame();//we will use the physical address
	TSS* tss = (TSS*)new_page;
	GDTR* gdtr = (GDTR*)(new_page + sizeof(TSS));

	global_gdt = alloc_frame();

	//null descriptor
	global_gdt[0] = make_gdt(0,0,0,0);
	//kernel code descriptor
	global_gdt[1] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_CONFORMING | GDT_READ_WRITE);
	//kernel data descriptor
	global_gdt[2] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_READ_WRITE);

	//user code descriptor
	global_gdt[3] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_READ_WRITE);
	//user data descriptor
	global_gdt[4] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_READ_WRITE);

	//the TSS and GDT is per CPU Physical Thread
	//TSS
	init_tss(tss);
	//alloc an emergency stack for an NMI or DOUBLE FAULT
	
	//TODO:
	//tss->ist1 = alloc_stack(INTERRUPT_STACK_SIZE);

	//rsp0-1-2 -> stack used when a lower CPL call higher CPL code
	//if it jumps to ring 0 will be used rsp0, ring1 -> rsp1 and so on
	//TODO:
	//void* cpl_change_stack = alloc_stack(SYSTEM_CALL_STACK_SIZE);//we use only one stack per physical thread
	void* cpl_change_stack = NULL;
	tss->rsp0 = cpl_change_stack;
	tss->rsp1 = cpl_change_stack;
	tss->rsp2 = cpl_change_stack;
	global_gdt[5] = make_gdt((uint64_t)tss, sizeof(TSS)-1, GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_TYPE_TSS_AVAIBLE);

	memory_fence();//force the flush of the cpu write buffer and Write Combining buffer
	load_gdt(global_gdt, gdt_size, gdtr);
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
		uint8_t ist = 0;//the ist used in the TSS, if set to 0 none is used
		uint8_t attributes_type = IDT_DPL(0) | IDT_PRESENT(1);

		if (i == DOUBLE_FAULT_INT_NUMBER || i == NMI_INT_NUMBER){
			//if it's an abort interrupt wich needs to have a good known stack
			//TODO:
			//ist = 1;
			ist = 0;
		}
		if(i < 32){
			//if it's an exception set it as a trap gate
			attributes_type |= IDT_TRAP_TYPE;
		}else
			attributes_type |= IDT_INTERRUPT_TYPE;

		idt_array[i] = make_idt_entry((void*)isr, KERNEL_CODE_SELECTOR, ist, attributes_type);
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
	init_cpu();
	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
