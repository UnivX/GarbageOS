#include "kernel_data.h"

typedef struct LocalKernelDataList{
	CPUID cpu_id;
	LocalKernelData local_data;
	struct LocalKernelDataList* next;
} LocalKernelDataList;

typedef struct KernelData{
	LocalKernelDataList* local_data_list;
	hard_spinlock lock;
	CPUID cpuid_to_allocate;
} KernelData;

KernelData kernel_data;

void init_kernel_data(){
	init_hard_spinlock(&kernel_data.lock);
	kernel_data.local_data_list = NULL;
	kernel_data.cpuid_to_allocate = 0;
}

CPUID register_local_kernel_data_cpu(){
	acquire_hard_spinlock(&kernel_data.lock);
	CPUID cpuid = kernel_data.cpuid_to_allocate;
		kernel_data.cpuid_to_allocate++;
	
	//if it's the first cpu
	if(kernel_data.local_data_list == NULL){
		kernel_data.local_data_list = kmalloc(sizeof(LocalKernelDataList));
		kernel_data.local_data_list->cpu_id = cpuid;
	}else{
		LocalKernelDataList* itr = kernel_data.local_data_list;
		while(itr->next != NULL)
			itr = itr->next;

		itr->next = kmalloc(sizeof(LocalKernelDataList));
		itr->next->cpu_id = cpuid;
	}
	release_hard_spinlock(&kernel_data.lock);
	return cpuid;
}
void set_local_kernel_data(CPUID cpu_id, LocalKernelData data){
	acquire_hard_spinlock(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->cpu_id == cpu_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	itr->local_data = data;
	release_hard_spinlock(&kernel_data.lock);
}
LocalKernelData get_local_kernel_data(CPUID cpu_id){
	LocalKernelData ldata;

	acquire_hard_spinlock(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->cpu_id == cpu_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	ldata = itr->local_data;
	release_hard_spinlock(&kernel_data.lock);
	return ldata;
}
CPUID get_cpu_id_from_apic_id(uint32_t apic_id){
	CPUID cpuid;

	acquire_hard_spinlock(&kernel_data.lock);
	LocalKernelDataList* itr = kernel_data.local_data_list;
	while(itr != NULL){
		if(itr->local_data.apic_id == apic_id)
			break;
		itr = itr->next;
	}
	KASSERT(itr != NULL);
	cpuid = itr->cpu_id;
	release_hard_spinlock(&kernel_data.lock);

	return cpuid;
}
