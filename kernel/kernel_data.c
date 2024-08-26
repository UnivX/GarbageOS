#include "kernel_data.h"

typedef struct LocalKernelDataList{
	CPUID cpu_id;
	LocalKernelData local_data;
	struct LocalKernelDataList* next;
} LocalKernelDataList;

typedef struct KernelData{
	LocalKernelDataList* local_data_list;
	spinlock lock;
	CPUID cpuid_to_allocate;
	PIT pit;
} KernelData;

KernelData kernel_data;

void init_kernel_data(){
	init_spinlock(&kernel_data.lock);
	kernel_data.local_data_list = NULL;
	kernel_data.cpuid_to_allocate = 0;
}

CPUID register_local_kernel_data_cpu(){
	ACQUIRE_SPINLOCK_HARD(&kernel_data.lock);
	CPUID cpuid = kernel_data.cpuid_to_allocate;
	kernel_data.cpuid_to_allocate++;
	
	//if it's the first cpu
	if(kernel_data.local_data_list == NULL){
		kernel_data.local_data_list = kmalloc(sizeof(LocalKernelDataList));
		kernel_data.local_data_list->cpu_id = cpuid;
		kernel_data.local_data_list->next = NULL;
		KASSERT(kernel_data.local_data_list != NULL);
	}else{
		LocalKernelDataList* itr = kernel_data.local_data_list;
		while(itr->next != NULL)
			itr = itr->next;
		KASSERT(itr != NULL);
		if(cpuid != 0)
		itr->next = kmalloc(sizeof(LocalKernelDataList));
		//freeze_cpu();
		KASSERT(itr->next != NULL);
		itr->next->cpu_id = cpuid;
		itr->next->next = NULL;
	}
	RELEASE_SPINLOCK_HARD(&kernel_data.lock);
	return cpuid;
}
void set_local_kernel_data(CPUID cpu_id, LocalKernelData data){
	ACQUIRE_SPINLOCK_HARD(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->cpu_id == cpu_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	itr->local_data = data;
	RELEASE_SPINLOCK_HARD(&kernel_data.lock);
}
LocalKernelData get_local_kernel_data(CPUID cpu_id){
	LocalKernelData ldata;

	ACQUIRE_SPINLOCK_HARD(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->cpu_id == cpu_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	ldata = itr->local_data;
	RELEASE_SPINLOCK_HARD(&kernel_data.lock);
	return ldata;
}
CPUID get_cpu_id_from_apic_id(uint32_t apic_id){
	CPUID cpuid;

	ACQUIRE_SPINLOCK_HARD(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->local_data.apic_id == apic_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	cpuid = itr->cpu_id;
	RELEASE_SPINLOCK_HARD(&kernel_data.lock);

	return cpuid;
}

void init_kernel_data_PIT(uint64_t target_frequency_hz){
	static atomic_uint is_pit_initialized = ATOMIC_VAR_INIT(0);
	if(atomic_load(&is_pit_initialized) != 0)
		kpanic_with_msg(GENERIC_ERROR, "init_kernel_data() called 2 times");
	atomic_fetch_add(&is_pit_initialized, 1);

	initialize_PIT_timer(&kernel_data.pit, target_frequency_hz, PIT_VECTOR);
}

PIT* get_PIT_from_kernel_data(){
	return &kernel_data.pit;
}
