#include "cpu.h"

extern uint64_t isr_stub_table[];
extern uint64_t isr_stub_table_end[];
extern void load_idt(IDTR*);
volatile GlobalCPUState global_cpu_state;

void make_CPU_states(){
	//at the moment we use only one physical thread
	const uint64_t number_of_logical_cores = 1;//TODO
	uint64_t gdt_entry_count = DATA_CODE_SELCTOR_NUMBER+1;//data-code selectors plus null one
	gdt_entry_count += number_of_logical_cores;//tss selectors

	//cpu structures sizes
	uint64_t gdt_size = sizeof(GDT)*gdt_entry_count;
	uint64_t idt_size = sizeof(IDTEntry)*INTERRUPT_NUMBER;
	uint64_t local_cpu_states_size = sizeof(LocalCPUState)*number_of_logical_cores;//also contain the tss
	uint64_t cpu_structures_real_size = gdt_size+idt_size+local_cpu_states_size;
	uint64_t cpu_structures_vmm_size = cpu_structures_real_size + PAGE_SIZE - (cpu_structures_real_size % PAGE_SIZE);

	//alloc the virtual memory
	//use one page on the upper and lower bound of memory to check for out of memory access
	global_cpu_state.cpu_structures_vmem = allocate_kernel_virtual_memory(cpu_structures_vmm_size, VM_TYPE_GENERAL_USE, PAGE_SIZE, PAGE_SIZE);
	
	global_cpu_state.gdt = (GDT*)get_vmem_addr(global_cpu_state.cpu_structures_vmem);
	global_cpu_state.gdt_entry_count = gdt_entry_count;
	global_cpu_state.idt = (IDTEntry*)((void*)global_cpu_state.gdt+gdt_size);
	global_cpu_state.local_cpu_states = (LocalCPUState*)((void*)global_cpu_state.idt+idt_size);

	//null descriptor
	global_cpu_state.gdt[0] = make_gdt(0,0,0,0);
	//kernel code descriptor
	global_cpu_state.gdt[1] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_CONFORMING | GDT_READ_WRITE);
	//kernel data descriptor
	global_cpu_state.gdt[2] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_NOT_SYSTEM | GDT_READ_WRITE);
	//user code descriptor
	global_cpu_state.gdt[3] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_EXECUTABLE | GDT_READ_WRITE);
	//user data descriptor
	global_cpu_state.gdt[4] = make_gdt(0, 0xffffff, GDT_GRANULARITY_FLAG | GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(3) | GDT_NOT_SYSTEM | GDT_READ_WRITE);

	//IDT setup
	IDTEntry* idt_array = global_cpu_state.idt;
	for(int i = 0; i < INTERRUPT_NUMBER; i++){
		uint64_t isr = isr_stub_table[i];
		//ist == 1 for exception stack(NMI and DOUBLE FAULT)
		//ist == 2 for page fault stack
		uint8_t ist = 0;//the ist used in the TSS, if set to 0 none is used
		uint8_t attributes_type = IDT_DPL(0) | IDT_PRESENT(1);

		if (i == DOUBLE_FAULT_INT_NUMBER || i == NMI_INT_NUMBER)
			ist = 1;
		else if (i == PAGE_FAULT_INT_NUMBER)
			ist = 2;

		//if it's an exception set it as a trap gate
		//and make it interruptible
		//if it's a normal interrupt make it ininterruptible
		if(i < 32)
			attributes_type |= IDT_TRAP_TYPE;
		else
			attributes_type |= IDT_INTERRUPT_TYPE;

		idt_array[i] = make_idt_entry((void*)isr, KERNEL_CODE_SELECTOR, ist, attributes_type);
	}
	global_cpu_state.idtr.idt = (uint64_t)&idt_array[0];
	global_cpu_state.idtr.size = (sizeof(IDTEntry)*INTERRUPT_NUMBER) -1;


	//set up the local cpu data
	for(uint64_t i = 0; i < number_of_logical_cores; i++){
		//alloc exception and page fault stacks
		global_cpu_state.local_cpu_states[i].exception_stack = allocate_kernel_virtual_memory(EXCEPTION_STACK_SIZE,
				VM_TYPE_STACK, CRITICAL_STACK_PADDING, CRITICAL_STACK_PADDING);
		global_cpu_state.local_cpu_states[i].page_fault_stack = allocate_kernel_virtual_memory(PAGE_FAULT_STACK_SIZE,
				VM_TYPE_STACK, CRITICAL_STACK_PADDING, CRITICAL_STACK_PADDING);

		TSS* local_tss = &global_cpu_state.local_cpu_states[i].tss;
		init_tss(local_tss);
		local_tss->ist1 =  get_vmem_addr(global_cpu_state.local_cpu_states[i].exception_stack);
		local_tss->ist1 += get_vmem_size(global_cpu_state.local_cpu_states[i].exception_stack);
		local_tss->ist2 =  get_vmem_addr(global_cpu_state.local_cpu_states[i].page_fault_stack);
		local_tss->ist2 += get_vmem_size(global_cpu_state.local_cpu_states[i].page_fault_stack);
		//the rsp for CPL change is relative to the task, we set it to null
		//so that if the kernel uses it it will throw a page fault
		local_tss->rsp0 = NULL;
		local_tss->rsp1 = NULL;
		local_tss->rsp2 = NULL;

		KASSERT((5+i)*sizeof(GDT) <= gdt_size);//check the gdt size;
		global_cpu_state.gdt[5+i] = make_gdt((uint64_t)local_tss, sizeof(TSS)-1, GDT_LONG_MODE_FLAG,
			GDT_PRESENT | GDT_DPL(0) | GDT_TYPE_TSS_AVAIBLE);
	}
	memory_fence();//force the flush of the cpu write buffer and Write Combining buffer
}

void load_CPU_state(){
	uint64_t local_cpu_id = 0;//TODO:
	volatile uint16_t tss_selector = get_tss_selector(local_cpu_id);

	load_gdt(global_cpu_state.gdt, global_cpu_state.gdt_entry_count, (GDTR*)&global_cpu_state.gdtr);

	load_idt((IDTR*)&global_cpu_state.idtr);

	asm volatile(
			"xor %%rax, %%rax\n"
			"mov %0, %%ax\n"
			"ltr %%ax\n"
			: : "r"(tss_selector) : "cc", "rax", "memory");//load the tss descriptor
}

uint16_t get_tss_selector(uint64_t cpu_id){
	uint16_t offset = cpu_id*0x10;
	return TSS_SELECTORS+offset;
}
