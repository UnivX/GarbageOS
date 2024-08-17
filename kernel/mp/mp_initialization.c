#include "mp_initialization.h"
#include "../hal.h"
#include "../interrupt/apic.h"
#include "../timer/pit.h"
#include "../HAL/bios/mp_init_bin.h"
#include "../mem/vmm.h"
#include "../util/sync_types.h"
#include "../kio.h"
#include "../kernel_data.h"
#include "../interrupt/interrupts.h""

static struct MPGlobalData{
	uint64_t initialized_APs;
	MPInitializationData init_data;
	APKernelData* APs_kernel_data;
	soft_spinlock lock;
} mp_gdata;

void AP_entry_point(void* loaded_stack){
	VMemHandle stack_vmem = NULL;

	acquire_soft_spinlock(&mp_gdata.lock);
	mp_gdata.initialized_APs++;
	for(uint64_t i = 0; i < mp_gdata.init_data.APs_count; i++){
		VMemHandle vmem = mp_gdata.APs_kernel_data[i].kernel_stack;
		if(get_vmem_addr(vmem)+get_vmem_size(vmem) == loaded_stack){
			stack_vmem = vmem;
			break;
		}
	}
	release_soft_spinlock(&mp_gdata.lock);

	KASSERT(stack_vmem != NULL);

	CPUID cpuid = register_local_kernel_data_cpu();
	init_cpu(cpuid);
	init_local_interrupt_controllers();

	LocalKernelData local_data = {stack_vmem, get_logical_core_lapic_id()};
	set_local_kernel_data(cpuid, local_data);
	printf("cpu %u64 started, loaded stack : %h64\n", (uint64_t)cpuid, (uint64_t)loaded_stack);
	enable_interrupts();

	while(1){
		halt();
	}
}

void init_APs(PIT* pit){
	init_soft_spinlock(&mp_gdata.lock);
	mp_gdata.initialized_APs = 0;
	mp_gdata.init_data.APs_count = get_number_of_usable_logical_cores()-1;
	if(mp_gdata.init_data.APs_count == 0)
		return;
	mp_gdata.init_data.stacks_top = kmalloc(sizeof(void*)*mp_gdata.init_data.APs_count);
	mp_gdata.init_data.ap_entry_point = AP_entry_point;

	mp_gdata.APs_kernel_data = kmalloc(sizeof(APKernelData)*mp_gdata.init_data.APs_count);

	for(uint64_t i = 0; i < mp_gdata.init_data.APs_count; i++){
		VMemHandle kstack = allocate_kernel_virtual_memory(KERNEL_STACK_SIZE, VM_TYPE_STACK, 16*KB, 64*MB);
		mp_gdata.APs_kernel_data[i].kernel_stack = kstack;
		mp_gdata.init_data.stacks_top[i] = get_vmem_addr(kstack)+get_vmem_size(kstack);//stack_top
	}
	volatile void* startup_frame = create_mp_init_routine_in_RAM(get_kernel_VMM_paging_structure(), (void*)&mp_gdata.init_data);
	uint8_t vector_addr = ((uint64_t)startup_frame) >> 12;

	send_IPI_INIT_to_all_excluding_self();
	PIT_wait_ms(pit, 10);
	send_startup_IPI_to_all_excluding_self(vector_addr);
	PIT_wait_us(pit, 200);
	send_startup_IPI_to_all_excluding_self(vector_addr);
	PIT_wait_us(pit, 200);
}
