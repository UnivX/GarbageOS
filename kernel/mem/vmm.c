#include "vmm.h"
#include "../hal.h"
#include "frame_allocator.h"
#include "heap.h"
#include "../kio.h"

static VirtualMemoryDescriptor* cut_descriptor(VirtualMemoryDescriptor* descriptor, void* start, uint64_t size);
static VirtualMemoryDescriptor* cut_descriptor_start(VirtualMemoryDescriptor* descriptor, uint64_t size);
static VirtualMemoryDescriptor* allocate_vmem_descriptor();
static void deallocate_vmm_descriptor(VirtualMemoryDescriptor* d);
static void update_head();
static const char* get_vm_type_string(VirtualMemoryType vm_type);

VirtualMemoryManager kernel_vmm;

//if the heap isn't already initialized return a 
static VirtualMemoryDescriptor* allocate_vmem_descriptor(){
	if(is_kheap_initialzed()){
		VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)kmalloc(sizeof(VirtualMemoryDescriptor));
		desc->is_from_heap = true;
	}

	static VirtualMemoryDescriptor bump_allocator[VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE];
	static uint64_t next_free_descriptor = 0;
	if(next_free_descriptor >= VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE)
		kpanic(VMM_ERROR);
	VirtualMemoryDescriptor* allocated_desc = &bump_allocator[next_free_descriptor];
	next_free_descriptor++;
	allocated_desc->is_from_heap = false;
	return allocated_desc;
}

static void deallocate_vmm_descriptor(VirtualMemoryDescriptor* d){
	if(d->is_from_heap)
		kfree((void*)d);
}

void initialize_kernel_VMM(void* paging_structure){
	VirtualMemoryDescriptor *first_desc = allocate_vmem_descriptor();
	VirtualMemoryDescriptor *second_desc = allocate_vmem_descriptor();

	first_desc->start_vaddr = (void*)PAGE_SIZE;//the first page is the nullptr page
	first_desc->size_bytes = (uint64_t)IDENTITY_MAP_VMEM_END - (uint64_t)first_desc->start_vaddr;
	first_desc->size_bytes -= first_desc->size_bytes % PAGE_SIZE;//round down

	first_desc->type = VM_TYPE_IDENTITY_MAP_FREE;
	first_desc->prev = NULL;
	first_desc->next = second_desc;

	second_desc->start_vaddr = KERNEL_VMEM_START;
	second_desc->size_bytes = (uint64_t)LAST_VADDR - (uint64_t)(second_desc->start_vaddr+1);//+1 for overflow error
	second_desc->size_bytes -= second_desc->size_bytes % PAGE_SIZE;//round down
																   
	second_desc->type = VM_TYPE_FREE;
	second_desc->prev = first_desc;
	second_desc->next = NULL;
	
	kernel_vmm.task_vm_list_head = first_desc;
	kernel_vmm.kernel_paging_structure = paging_structure;
}

bool identity_map(void* paddr, uint64_t size){
	KASSERT(paddr != NULL);
	KASSERT(size % PAGE_SIZE == 0);
	KASSERT((uint64_t)paddr % PAGE_SIZE == 0);

	VirtualMemoryDescriptor *descriptor = kernel_vmm.task_vm_list_head;

	//find the correct descriptor
	while(descriptor->next != NULL && (descriptor->start_vaddr + descriptor->size_bytes) < paddr)
		descriptor = descriptor->next;

	//if the descriptor doesnt contain the requested range then return false
	//(there cannot be two contiguos consecutive descriptors
	if(paddr < descriptor->start_vaddr || paddr+size > descriptor->start_vaddr+descriptor->size_bytes)
		return false;

	if(descriptor->type != VM_TYPE_IDENTITY_MAP_FREE)
		return false;

	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor(descriptor, paddr, size);
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);
	cutted_descriptor->type = VM_TYPE_IDENTITY_MAP;
	for(void* to_map = cutted_descriptor->start_vaddr; to_map < cutted_descriptor->start_vaddr+cutted_descriptor->size_bytes; to_map+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, to_map, to_map, PAGE_WRITABLE | PAGE_PRESENT);
	}
	
	return true;
}

void* memory_map(void* paddr, uint64_t size, uint16_t page_flags){
	KASSERT(paddr != NULL);
	KASSERT(size % PAGE_SIZE == 0);
	KASSERT((uint64_t)paddr % PAGE_SIZE == 0);

	VirtualMemoryDescriptor *descriptor = kernel_vmm.task_vm_list_head;

	while( !(descriptor->size_bytes >= size && descriptor->type == VM_TYPE_FREE) && descriptor->next != NULL)
		descriptor = descriptor->next;

	if(descriptor->type != VM_TYPE_FREE){
		kpanic(VMM_ERROR);
		return NULL;
	}

	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor_start(descriptor, size);
	cutted_descriptor->type = VM_TYPE_MEMORY_MAPPING;
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);

	void* vaddr = cutted_descriptor->start_vaddr;
	for(uint64_t i = 0; i < size; i+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, vaddr+i, paddr+i, page_flags | PAGE_PRESENT);
	}

	return cutted_descriptor->start_vaddr;
}

bool copy_memory_mapping_from_paging_structure(void* src_paging_structure, void* vaddr, uint64_t size, uint16_t page_flags){
	VirtualMemoryDescriptor *descriptor = kernel_vmm.task_vm_list_head;

	//find the correct descriptor
	while(descriptor->next != NULL && (descriptor->start_vaddr + descriptor->size_bytes) < vaddr)
		descriptor = descriptor->next;

	//if the descriptor doesnt contain the requested range then return false
	//(there cannot be two contiguos consecutive descriptors
	if(vaddr < descriptor->start_vaddr || vaddr+size > descriptor->start_vaddr+descriptor->size_bytes)
		return false;

	if(descriptor->type != VM_TYPE_FREE)
		return false;
	
	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor_start(descriptor, size);
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);
	cutted_descriptor->type = VM_TYPE_MEMORY_MAPPING;

	if(page_flags == COPY_FLAGS_ON_MMAP_COPY)
		page_flags = 0;

	copy_paging_structure_mapping_no_page_invalidation(src_paging_structure, kernel_vmm.kernel_paging_structure, vaddr, size, page_flags);
	return true;
}

void* allocate_kernel_virtual_memory(uint64_t size, VirtualMemoryType type){
	KASSERT(size % PAGE_SIZE == 0);
	bool is_valid = type != VM_TYPE_IDENTITY_MAP_FREE && type != VM_TYPE_IDENTITY_MAP;

	if(!is_valid){
		kpanic(VMM_ERROR);
		return NULL;
	}

	VirtualMemoryDescriptor *descriptor = kernel_vmm.task_vm_list_head;

	while( !(descriptor->size_bytes >= size && descriptor->type == VM_TYPE_FREE) && descriptor->next != NULL)
		descriptor = descriptor->next;

	if(descriptor->type != VM_TYPE_FREE){
		kpanic(VMM_ERROR);
		return NULL;
	}

	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor_start(descriptor, size);
	cutted_descriptor->type = type;
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);

	void* vaddr = cutted_descriptor->start_vaddr;
	for(uint64_t i = 0; i < size; i+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, vaddr, alloc_frame(), PAGE_PRESENT | PAGE_WRITABLE);
		vaddr += PAGE_SIZE;
	}
	return cutted_descriptor->start_vaddr;
}

static VirtualMemoryDescriptor* cut_descriptor(VirtualMemoryDescriptor* descriptor, void* start, uint64_t size){
	KASSERT(descriptor->type == VM_TYPE_FREE || descriptor->type == VM_TYPE_IDENTITY_MAP_FREE);
	KASSERT(descriptor->size_bytes % PAGE_SIZE == 0);
	KASSERT(size % PAGE_SIZE == 0);
	VirtualMemoryDescriptor* old_prev = descriptor->prev;
	VirtualMemoryDescriptor* old_next = descriptor->next;

	if(descriptor->start_vaddr == start && descriptor->size_bytes == size){
		return descriptor;
	}else if(descriptor->start_vaddr == start){
		/*
		VirtualMemoryDescriptor *new_descriptor = allocate_vmem_descriptor();

		uint64_t new_size = descriptor->size_bytes - size;
		descriptor->size_bytes = size;
		new_descriptor->size_bytes = new_size;
		new_descriptor->type = descriptor->type;
		new_descriptor->start_vaddr = descriptor->start_vaddr + descriptor->size_bytes;

		//set the linked list
		//old_prev -> descriptor -> new_descriptor -> old_next
		descriptor->prev = old_prev;
		descriptor->next = new_descriptor;
		new_descriptor->next = old_next;
		new_descriptor->prev = descriptor;
		KASSERT(descriptor->start_vaddr+descriptor->size_bytes == new_descriptor->start_vaddr);

		return descriptor;
		*/
		return cut_descriptor_start(descriptor, size);
	}else if(descriptor->start_vaddr + descriptor->size_bytes == start + size){
		VirtualMemoryDescriptor *new_descriptor = allocate_vmem_descriptor();

		uint64_t new_size = descriptor->size_bytes - size;
		descriptor->size_bytes = new_size;
		new_descriptor->size_bytes = size;
		new_descriptor->start_vaddr = descriptor->start_vaddr + descriptor->size_bytes;
		new_descriptor->type = descriptor->type;
		KASSERT(new_descriptor->start_vaddr == start);

		//set the linked list
		//old_prev -> descriptor -> new_descriptor -> old_next
		descriptor->prev = old_prev;
		descriptor->next = new_descriptor;
		new_descriptor->next = old_next;
		new_descriptor->prev = descriptor;
		KASSERT(descriptor->start_vaddr+descriptor->size_bytes == new_descriptor->start_vaddr);
		update_head();
		return new_descriptor;
	}else{
		KASSERT(start > descriptor->start_vaddr);
		uint64_t old_size = descriptor->size_bytes;
		void* old_start_vaddr = descriptor->start_vaddr;
		VirtualMemoryDescriptor *prev_descriptor = allocate_vmem_descriptor();
		VirtualMemoryDescriptor *mid_descriptor = descriptor;
		VirtualMemoryDescriptor *next_descriptor = allocate_vmem_descriptor();

		prev_descriptor->type = descriptor->type;
		next_descriptor->type = descriptor->type;

		prev_descriptor->prev = old_prev;
		prev_descriptor->next = mid_descriptor;
		mid_descriptor->prev = prev_descriptor;
		mid_descriptor->next = next_descriptor;
		next_descriptor->prev = mid_descriptor;
		next_descriptor->next = old_next;

		prev_descriptor->start_vaddr = descriptor->start_vaddr;
		prev_descriptor->size_bytes = start-descriptor->start_vaddr;
		KASSERT(prev_descriptor->start_vaddr+prev_descriptor->size_bytes == start);
		mid_descriptor->start_vaddr = start;
		mid_descriptor->size_bytes = size;
		next_descriptor->start_vaddr = mid_descriptor->start_vaddr + mid_descriptor->size_bytes;
		next_descriptor->size_bytes = (uint64_t)old_start_vaddr+old_size - ((uint64_t)start + size);
		KASSERT(old_start_vaddr+old_size == next_descriptor->start_vaddr+next_descriptor->size_bytes);
		update_head();
		return mid_descriptor;
	}
	KASSERT(false);//should never be here
	return NULL;
}

static VirtualMemoryDescriptor* cut_descriptor_start(VirtualMemoryDescriptor* descriptor, uint64_t size){
	KASSERT(descriptor->type == VM_TYPE_FREE || descriptor->type == VM_TYPE_IDENTITY_MAP_FREE);
	KASSERT(descriptor->size_bytes > size)
	KASSERT(descriptor->size_bytes % PAGE_SIZE == 0);
	KASSERT(size % PAGE_SIZE == 0);
	VirtualMemoryDescriptor* old_prev = descriptor->prev;
	VirtualMemoryDescriptor* old_next = descriptor->next;

	if(descriptor->size_bytes == size)
		return descriptor;
	
	VirtualMemoryDescriptor *new_descriptor = allocate_vmem_descriptor();

	uint64_t new_size = descriptor->size_bytes - size;
	descriptor->size_bytes = size;
	new_descriptor->size_bytes = new_size;
	new_descriptor->type = descriptor->type;
	new_descriptor->start_vaddr = descriptor->start_vaddr + descriptor->size_bytes;

	//set the linked list
	//old_prev -> descriptor -> new_descriptor -> old_next
	descriptor->prev = old_prev;
	descriptor->next = new_descriptor;
	new_descriptor->next = old_next;
	new_descriptor->prev = descriptor;
	KASSERT(descriptor->start_vaddr+descriptor->size_bytes == new_descriptor->start_vaddr);
	update_head();

	return descriptor;
}

static void update_head(){
	while(kernel_vmm.task_vm_list_head->prev != NULL)
		kernel_vmm.task_vm_list_head = kernel_vmm.task_vm_list_head->prev;
}

static const char* get_vm_type_string(VirtualMemoryType vm_type){
	switch(vm_type){
		case VM_TYPE_FREE:
			return "FREE";
		break;
		case VM_TYPE_IDENTITY_MAP_FREE:
			return "IDENTITY MAP FREE";
		break;
		case VM_TYPE_STACK:
			return "STACK";
		break;
		case VM_TYPE_HEAP:
			return "HEAP";
		break;
		case VM_TYPE_GENERAL_USE:
			return "GENERAL USE";
		break;
		case VM_TYPE_MEMORY_MAPPING:
			return "MEMORY MAPPING";
		break;
		case VM_TYPE_IDENTITY_MAP:
			return "IDENTITY MAP";
		break;
	}
	return "UNKNOWN";
}

void debug_print_kernel_vmm(){
	print("\n-----kernel VMM state start-----\n");
	print("kernel paging structure: ");
	print_uint64_hex((uint64_t)kernel_vmm.kernel_paging_structure);
	print("\n");
	print("virtual adresses:\n");

	for(VirtualMemoryDescriptor* d = kernel_vmm.task_vm_list_head; d != NULL; d = d->next){
		print_uint64_hex((uint64_t)d->start_vaddr);
		print(" / ");
		print_uint64_hex((uint64_t)d->start_vaddr + d->size_bytes);
		print(" -> ");
		print(get_vm_type_string(d->type));
		print("\n");
	}
	print("-----kernel VMM state end-----\n");
}
