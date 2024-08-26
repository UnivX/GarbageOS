#include "mp_initialization.h"
#include "../hal.h"
#include "../interrupt/apic.h"
#include "../timer/pit.h"
#include "../HAL/bios/mp_init_bin.h"
#include "../mem/vmm.h"
#include "../mem/vaddr_cache_shootdown.h"
#include "../util/sync_types.h"
#include "../kio.h"
#include "../kernel_data.h"
#include "../interrupt/interrupts.h"

static struct MPGlobalData{
	atomic_uint initialized_APs;
	MPInitializationData init_data;
	APKernelData* APs_kernel_data;
	spinlock lock;
} mp_gdata;

void AP_entry_point(void* loaded_stack){
	VMemHandle stack_vmem = NULL;

	acquire_spinlock(&mp_gdata.lock);
	for(uint64_t i = 0; i < mp_gdata.init_data.APs_count; i++){
		VMemHandle vmem = mp_gdata.APs_kernel_data[i].kernel_stack;
		if(get_vmem_addr(vmem)+get_vmem_size(vmem) == loaded_stack){
			stack_vmem = vmem;
			break;
		}
	}
	release_spinlock(&mp_gdata.lock);

	//set the real paging structure
	set_active_paging_structure((volatile void*)get_kernel_VMM_paging_structure());
	enable_PAT();
	//init_cpu(0);
	//freeze_cpu();

	CPUID cpuid = register_local_kernel_data_cpu();
	init_cpu(cpuid);

	KASSERT(stack_vmem != NULL);

	init_local_interrupt_controllers();

	LocalKernelData local_data = {stack_vmem, get_logical_core_lapic_id()};
	set_local_kernel_data(cpuid, local_data);

	activate_this_cpu_vmmcache_shootdown();
#ifdef DEBUG
	printf("cpu %u64 started, loaded stack : %h64\n", (uint64_t)cpuid, (uint64_t)loaded_stack);
#endif
	kio_flush();
	enable_interrupts();

	acquire_spinlock(&mp_gdata.lock);
	atomic_fetch_add(&mp_gdata.initialized_APs, 1);//increment
	release_spinlock(&mp_gdata.lock);

	PIT* pit = get_PIT_from_kernel_data();
	PIT_wait_ms(pit, 3000);
	printf("3 seconds have passed for cpu %u32\n", (uint32_t)cpuid);
	kio_flush();

	while(1){
		halt();
	}
}

MPInitError unsync_set_up_mp_global_data(){
	atomic_store(&mp_gdata.initialized_APs, 0);
	mp_gdata.init_data.APs_count = get_number_of_usable_logical_cores()-1;
	if(mp_gdata.init_data.APs_count == 0)
		return ERROR_MP_OK;
	mp_gdata.init_data.stacks_top = kmalloc(sizeof(void*)*mp_gdata.init_data.APs_count);
	mp_gdata.init_data.ap_entry_point = AP_entry_point;

	mp_gdata.APs_kernel_data = kmalloc(sizeof(APKernelData)*mp_gdata.init_data.APs_count);

	for(uint64_t i = 0; i < mp_gdata.init_data.APs_count; i++){
		VMemHandle kstack = allocate_kernel_virtual_memory(KERNEL_STACK_SIZE, VM_TYPE_STACK, 16*KB, 64*MB);
		mp_gdata.APs_kernel_data[i].kernel_stack = kstack;
		mp_gdata.init_data.stacks_top[i] = get_vmem_addr(kstack)+get_vmem_size(kstack);//stack_top
	}
	return ERROR_MP_OK;
}

MPInitError init_APs(PIT* pit){
	//no APs to startup
	if(get_number_of_usable_logical_cores() == 1)
		return ERROR_MP_OK;

	init_spinlock(&mp_gdata.lock);

	MPInitError mp_gdata_setup_err = unsync_set_up_mp_global_data();
	if(mp_gdata_setup_err != ERROR_MP_OK)
		return mp_gdata_setup_err;

	//generate the trampoline code to jump to the init AP function
	volatile void* startup_frame = create_mp_init_routine_in_RAM(get_kernel_VMM_paging_structure(), (void*)&mp_gdata.init_data);
	uint8_t vector_addr = ((uint64_t)startup_frame) >> 12;

	//get the lapic ids
	uint64_t number_of_logical_cores = get_number_of_usable_logical_cores();
	uint32_t* lapic_ids = kmalloc(number_of_logical_cores*sizeof(uint32_t));
	int64_t array_result = get_lapic_id_array(lapic_ids, number_of_logical_cores);
	if(array_result < 0 || (uint64_t)array_result != number_of_logical_cores)
		kpanic(GENERIC_ERROR);

	//DO THE INIT SIPI SIPI initialization
	send_IPI_INIT_to_all_excluding_self();
	PIT_wait_ms(pit, 10);
	uint32_t BSP_lapic_id = get_logical_core_lapic_id();
	for(uint64_t i = 0; i < number_of_logical_cores; i++){
		if(lapic_ids[i] == BSP_lapic_id)
			continue;
#ifdef DEBUG
		printf("starting %u64\n", (uint64_t)lapic_ids[i]);
#endif
		kio_flush();
		send_startup_IPI(lapic_ids[i], vector_addr);
		PIT_wait_us(pit, 200);
		send_startup_IPI(lapic_ids[i], vector_addr);
		PIT_wait_us(pit, 200);
	}

	//wait for CPUs initialization
	const uint64_t max_attempts = 30;
	const uint64_t attempt_ms_wait = 1;
	uint64_t attempts_count = 0;
	uint64_t number_of_APs = number_of_logical_cores-1;//-BSP
	while(attempts_count < max_attempts){
		uint64_t initialized_APs_copy = atomic_load(&mp_gdata.initialized_APs);
		if(initialized_APs_copy < number_of_APs)
			PIT_wait_ms(pit, attempt_ms_wait);
		else if(initialized_APs_copy > number_of_APs)
			kpanic(GENERIC_ERROR);
		else
			return ERROR_MP_OK;
	}

	uint64_t initialized_APs_copy = atomic_load(&mp_gdata.initialized_APs);
	if(initialized_APs_copy == number_of_APs)
		return ERROR_MP_OK;
	if(initialized_APs_copy > number_of_APs)
		kpanic(GENERIC_ERROR);
	if(initialized_APs_copy > 0)
		return ERROR_MP_PARTIAL_STARTUP;
	return ERROR_MP_STARTUP_FAILED;
}
