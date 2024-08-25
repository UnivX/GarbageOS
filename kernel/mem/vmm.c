#include "vmm.h"
#include "../hal.h"
#include "frame_allocator.h"
#include "../interrupt/interrupts.h"
#include "heap.h"
#include "../kio.h"
#include "vaddr_cache_shootdown.h"

static VirtualMemoryManager kernel_vmm;

static VirtualMemoryDescriptor* cut_descriptor(VirtualMemoryDescriptor* descriptor, void* start, uint64_t size);
static VirtualMemoryDescriptor* cut_descriptor_start(VirtualMemoryDescriptor* descriptor, uint64_t size);
static VirtualMemoryDescriptor* allocate_vmem_descriptor();
static void deallocate_vmm_descriptor(VirtualMemoryDescriptor* d);
static void update_head();
static const char* get_vm_type_string(VirtualMemoryType vm_type);
static uint64_t get_no_padding_size(const VirtualMemoryDescriptor* d);
static void* get_no_padding_start_addr(const VirtualMemoryDescriptor* d);
void page_fault(InterruptInfo info);

//if the heap isn't already initialized return a bump allocated one
static VirtualMemoryDescriptor* allocate_vmem_descriptor(){
	VirtualMemoryDescriptor* allocated_desc = NULL;
	if(is_kheap_initialzed()){
		allocated_desc = (VirtualMemoryDescriptor*)kmalloc(sizeof(VirtualMemoryDescriptor));
		allocated_desc->is_from_heap = true;
	}else{
		static VirtualMemoryDescriptor bump_allocator[VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE];
		static uint64_t next_free_descriptor = 0;
		if(next_free_descriptor >= VIRTUAL_MEMORY_DESCRIPTOR_EARLY_ALLOCATION_SIZE)
			kpanic(VMM_ERROR);
		allocated_desc = &bump_allocator[next_free_descriptor];
		next_free_descriptor++;
		allocated_desc->is_from_heap = false;
	}
	allocated_desc->lower_padding = 0;
	allocated_desc->upper_padding = 0;
	return allocated_desc;
}

static void deallocate_vmm_descriptor(VirtualMemoryDescriptor* d){
	if(d->is_from_heap)
		kfree((void*)d);
}

void initialize_kernel_VMM(void* paging_structure){
	install_interrupt_handler(0xe, page_fault);
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
	
	kernel_vmm.vm_list_head = first_desc;
	init_spinlock(&kernel_vmm.lock);
	kernel_vmm.kernel_paging_structure = paging_structure;
}

const void* get_kernel_VMM_paging_structure(){
	//readonly pointer, there is no need for the spinlock
	return kernel_vmm.kernel_paging_structure;
}

VMemHandle identity_map(void* paddr, uint64_t size){
	KASSERT(paddr != NULL);
	KASSERT(size % PAGE_SIZE == 0);
	KASSERT((uint64_t)paddr % PAGE_SIZE == 0);

	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	VMemHandle result_handle = NULL;
	VirtualMemoryDescriptor *descriptor = kernel_vmm.vm_list_head;

	//find the correct descriptor
	while(descriptor->next != NULL && (descriptor->start_vaddr + descriptor->size_bytes) <= paddr)
		descriptor = descriptor->next;

	//if the descriptor doesnt contain the requested range then return false
	//(there cannot be two contiguos consecutive descriptors
	if(paddr < descriptor->start_vaddr || paddr+size > descriptor->start_vaddr+descriptor->size_bytes){
		result_handle = NULL;
	}else if(descriptor->type != VM_TYPE_IDENTITY_MAP_FREE){
		result_handle = NULL;
	}else{
		VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor(descriptor, paddr, size);
		KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);
		cutted_descriptor->type = VM_TYPE_IDENTITY_MAP;
		/*
		for(void* to_map = cutted_descriptor->start_vaddr; to_map < cutted_descriptor->start_vaddr+cutted_descriptor->size_bytes; to_map+=PAGE_SIZE){
			paging_map(kernel_vmm.kernel_paging_structure, to_map, to_map, PAGE_WRITABLE | PAGE_PRESENT);
		}
		*/
	
		result_handle = (VMemHandle)cutted_descriptor;
	}
	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
	return result_handle;
}

void load_identity_map_pages(void* paddr, uint64_t size, VMemHandle handle){
	KASSERT(size%PAGE_SIZE == 0);
	KASSERT((uint64_t)paddr%PAGE_SIZE == 0);
	//TODO: check if we can lock and unlock the spinlock in the for loop

	/*
	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);

	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)&handle;
	if(paddr < get_no_padding_start_addr(desc))
		kpanic(VMM_ERROR);
	if(paddr + size > get_no_padding_start_addr(desc) + get_no_padding_size(desc))
		kpanic(VMM_ERROR);

	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
	*/

	for(void* to_map = paddr; to_map < paddr+size; to_map+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, to_map, to_map, PAGE_WRITABLE | PAGE_PRESENT);
	}
	VMMCacheShootdownRange shootdown_range;
	shootdown_range.addr_start = paddr;
	shootdown_range.size_in_pages = size/PAGE_SIZE;
	if(is_vmmcache_shootdown_subsystem_initialized())
		vmmcache_shootdown(shootdown_range);
}

VMemHandle memory_map(void* paddr, uint64_t size, uint16_t page_flags){
	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
#ifdef MEMORY_MAP_PADDING
	size += MEMORY_MAP_PADDING*2;
#endif

	//KASSERT(paddr != NULL);
	KASSERT(size % PAGE_SIZE == 0);
	KASSERT((uint64_t)paddr % PAGE_SIZE == 0);

	VirtualMemoryDescriptor *descriptor = kernel_vmm.vm_list_head;

	while( !(descriptor->size_bytes >= size && descriptor->type == VM_TYPE_FREE) && descriptor->next != NULL)
		descriptor = descriptor->next;

	if(descriptor->type != VM_TYPE_FREE){
		//no need to unlock the spinlock because we are going to panic
		kpanic(VMM_ERROR);
		return NULL;
	}

	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor_start(descriptor, size);
	cutted_descriptor->type = VM_TYPE_MEMORY_MAPPING;
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);

#ifdef MEMORY_MAP_PADDING
	cutted_descriptor->lower_padding = MEMORY_MAP_PADDING;
	cutted_descriptor->upper_padding = MEMORY_MAP_PADDING;
#endif

	void* vaddr = get_no_padding_start_addr(cutted_descriptor);
	uint64_t no_padding_size = get_no_padding_size(cutted_descriptor);
	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);

	for(uint64_t i = 0; i < no_padding_size; i+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, vaddr+i, paddr+i, page_flags | PAGE_PRESENT);
	}

	VMMCacheShootdownRange shootdown_range;
	shootdown_range.addr_start = vaddr;
	shootdown_range.size_in_pages = no_padding_size/PAGE_SIZE;
	if(is_vmmcache_shootdown_subsystem_initialized())
		vmmcache_shootdown(shootdown_range);

	return (VMemHandle)cutted_descriptor;
}

VMemHandle copy_memory_mapping_from_paging_structure(void* src_paging_structure, void* vaddr, uint64_t size, uint16_t page_flags){
	KASSERT(size % PAGE_SIZE == 0);
	VMemHandle result = NULL;
	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	VirtualMemoryDescriptor *descriptor = kernel_vmm.vm_list_head;

	//find the correct descriptor
	while(descriptor->next != NULL && (descriptor->start_vaddr + descriptor->size_bytes) <= vaddr)
		descriptor = descriptor->next;

	//if the descriptor doesnt contain the requested range then return false
	//(there cannot be two contiguos consecutive descriptors
	if(vaddr < descriptor->start_vaddr || vaddr+size > descriptor->start_vaddr+descriptor->size_bytes){
		result = NULL;
	}else if(descriptor->type != VM_TYPE_FREE){
		result = NULL;
	}else{
		VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor(descriptor, vaddr, size);
		KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);
		cutted_descriptor->type = VM_TYPE_MEMORY_MAPPING;

		if(page_flags == COPY_FLAGS_ON_MMAP_COPY)
			page_flags = 0;

		RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
		copy_paging_structure_mapping_no_page_invalidation(src_paging_structure, kernel_vmm.kernel_paging_structure, vaddr, size, page_flags);
		//invalidate translation cache
		for(uint64_t i = 0; i < size; i+=PAGE_SIZE){
			uint64_t translation_vaddr = (uint64_t)(vaddr)+i;
			delete_page_translation_cache((void*)translation_vaddr);
		}

		VMMCacheShootdownRange shootdown_range;
		shootdown_range.addr_start = vaddr;
		shootdown_range.size_in_pages = size/PAGE_SIZE;
		if(is_vmmcache_shootdown_subsystem_initialized())
			vmmcache_shootdown(shootdown_range);

		REACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
		result = (VMemHandle)cutted_descriptor;
	}
	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
	return result;
}

VMemHandle allocate_kernel_virtual_memory(uint64_t size, VirtualMemoryType type, uint64_t upper_padding, uint64_t lower_padding){
	size += lower_padding + upper_padding;
	KASSERT(size % PAGE_SIZE == 0);
	bool is_valid = type != VM_TYPE_IDENTITY_MAP_FREE && type != VM_TYPE_IDENTITY_MAP && type != VM_TYPE_MEMORY_MAPPING;

	if(!is_valid){
		kpanic(VMM_ERROR);
		return NULL;
	}
	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	VirtualMemoryDescriptor *descriptor = kernel_vmm.vm_list_head;

	while( !(descriptor->size_bytes >= size && descriptor->type == VM_TYPE_FREE) && descriptor->next != NULL)
		descriptor = descriptor->next;

	if(descriptor->type != VM_TYPE_FREE){
		//no need to release the spinlock because we are going to panic
		kpanic(VMM_ERROR);
		return NULL;
	}

	VirtualMemoryDescriptor* cutted_descriptor = cut_descriptor_start(descriptor, size);
	cutted_descriptor->type = type;
	KASSERT((uint64_t)cutted_descriptor->start_vaddr % PAGE_SIZE == 0);
	cutted_descriptor->lower_padding = lower_padding;
	cutted_descriptor->upper_padding = upper_padding;

	void* vaddr = get_no_padding_start_addr(cutted_descriptor);
	uint64_t no_padding_size = get_no_padding_size(cutted_descriptor);
	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
	for(uint64_t i = 0; i < no_padding_size; i+=PAGE_SIZE){
		paging_map(kernel_vmm.kernel_paging_structure, vaddr, alloc_frame(), PAGE_PRESENT | PAGE_WRITABLE);
		vaddr += PAGE_SIZE;
	}

	VMMCacheShootdownRange shootdown_range;
	shootdown_range.addr_start = vaddr;
	shootdown_range.size_in_pages = no_padding_size/PAGE_SIZE;
	if(is_vmmcache_shootdown_subsystem_initialized())
		vmmcache_shootdown(shootdown_range);

	return (VMemHandle)cutted_descriptor;
}

bool deallocate_kernel_virtual_memory(VMemHandle handle){
	ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;
	KASSERT(get_vmem_type(desc) != VM_TYPE_FREE);
	KASSERT(get_vmem_type(desc) != VM_TYPE_IDENTITY_MAP_FREE);
	//get the free type based on the vaddr
	VirtualMemoryType free_type = desc->start_vaddr < IDENTITY_MAP_VMEM_END ? VM_TYPE_IDENTITY_MAP_FREE : VM_TYPE_FREE;

	
	//i don't know WTH is this if
	//if(!free_type){
	{
		//invalidate and free virtual memory
		void* vaddr = get_no_padding_start_addr(desc);
		for(uint64_t i = 0; i < get_no_padding_size(desc); i+=PAGE_SIZE){
			if(desc->type == VM_TYPE_HEAP || desc->type == VM_TYPE_STACK || desc->type == VM_TYPE_GENERAL_USE){
				PagingMapState state = get_physical_address(kernel_vmm.kernel_paging_structure, vaddr);
				if(state.mmap_present)
					dealloc_frame(state.paddr);
			}
			invalidate_paging_mapping(kernel_vmm.kernel_paging_structure, vaddr);
			vaddr += PAGE_SIZE;
		}
		VMMCacheShootdownRange shootdown_range;
		shootdown_range.addr_start = vaddr;
		shootdown_range.size_in_pages = get_no_padding_size(desc) /PAGE_SIZE;
		if(is_vmmcache_shootdown_subsystem_initialized())
			vmmcache_shootdown(shootdown_range);
	}
	
	//set the range as a free one
	desc->type = free_type;
	desc->lower_padding = 0;
	desc->upper_padding = 0;

	//if the prev range is of a free type then set the desc to the prev
	if(desc->prev != NULL)
		if(desc->prev->type == free_type)
			desc = desc->prev;

	//if the next is not free return
	if(desc->next->type == free_type){
		//calculate the total new size
		//and deallocate the free ranges
		uint64_t total_size = desc->size_bytes;
		VirtualMemoryDescriptor* last_range = desc->next;
		while(last_range != NULL){
			if(last_range->type != free_type)
				break;
			total_size += last_range->size_bytes;

			VirtualMemoryDescriptor* to_deallocate = last_range;
			last_range = last_range->next;
			deallocate_vmm_descriptor(to_deallocate);
		}

		//set the new linked list
		desc->size_bytes = total_size;
		desc->next = last_range;
		if(last_range != NULL)
			last_range->prev = desc;

		//in theory it's not needed to refresh the head of the linked list
	}
	RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
	return true;
}

bool _try_expand_vmem_top(VMemHandle handle, uint64_t size, bool use_spinlock){
	
	//adjust size
	if(size % PAGE_SIZE != 0)
		size += PAGE_SIZE - (size % PAGE_SIZE);


	//equivalent to 
	//ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	InterruptState lock_istate;
	if(use_spinlock)
		acquire_spinlock_hard(&kernel_vmm.lock, &lock_istate);


	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;

	bool is_type_invalid = desc->type != VM_TYPE_HEAP && desc->type != VM_TYPE_STACK && desc->type != VM_TYPE_GENERAL_USE;
	bool is_too_small = size > desc->upper_padding;
	bool not_expandable = is_type_invalid || is_too_small ;
	
	//map the memory to some free frames
	void* vaddr = get_no_padding_start_addr(desc) + get_no_padding_size(desc);
	void* desc_end_vaddr = (desc->start_vaddr + desc->size_bytes);
		
	
	if(!not_expandable){
		//set the new padding
		desc->upper_padding -= size;
		//equivalent to
		//RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
		if(use_spinlock)
			release_spinlock_hard(&kernel_vmm.lock, &lock_istate);
		for(uint64_t i = 0; i < size; i+=PAGE_SIZE){
			KASSERT(vaddr < desc_end_vaddr);
			paging_map(kernel_vmm.kernel_paging_structure, vaddr, alloc_frame(), PAGE_PRESENT | PAGE_WRITABLE);
			vaddr += PAGE_SIZE;
		}
		VMMCacheShootdownRange shootdown_range;
		shootdown_range.addr_start = vaddr;
		shootdown_range.size_in_pages = size/PAGE_SIZE;
		if(is_vmmcache_shootdown_subsystem_initialized())
			vmmcache_shootdown(shootdown_range);
	}

	return !not_expandable;
}

bool try_expand_vmem_top(VMemHandle handle, uint64_t size){
	return _try_expand_vmem_top(handle, size, true); 
}

bool _try_expand_vmem_bottom(VMemHandle handle, uint64_t size, bool use_spinlock){
	
	//adjust size
	if(size % PAGE_SIZE != 0)
		size += PAGE_SIZE - (size % PAGE_SIZE);

	//equivalent to 
	//ACQUIRE_SPINLOCK_HARD(&kernel_vmm.lock);
	InterruptState lock_istate;
	if(use_spinlock)
		acquire_spinlock_hard(&kernel_vmm.lock, &lock_istate);


	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;

	bool is_type_invalid = desc->type != VM_TYPE_HEAP && desc->type != VM_TYPE_STACK && desc->type != VM_TYPE_GENERAL_USE;
	bool is_too_small = size > desc->lower_padding;
	bool not_expandable = is_type_invalid || is_too_small ;
	
	//map the memory to some free frames
	void* vaddr = get_no_padding_start_addr(desc)-size;
	void* desc_start_vaddr = desc->start_vaddr;
		
	if(!not_expandable){
		//set the new padding
		desc->lower_padding -= size;
		//equivalent to
		//RELEASE_SPINLOCK_HARD(&kernel_vmm.lock);
		if(use_spinlock)
			release_spinlock_hard(&kernel_vmm.lock, &lock_istate);
		for(uint64_t i = 0; i < size; i+=PAGE_SIZE){
			KASSERT(vaddr >= desc_start_vaddr);
			paging_map(kernel_vmm.kernel_paging_structure, vaddr, alloc_frame(), PAGE_PRESENT | PAGE_WRITABLE);
			vaddr += PAGE_SIZE;
		}
		VMMCacheShootdownRange shootdown_range;
		shootdown_range.addr_start = vaddr;
		shootdown_range.size_in_pages = size/PAGE_SIZE;
		if(is_vmmcache_shootdown_subsystem_initialized())
			vmmcache_shootdown(shootdown_range);
	}
	return !not_expandable;
}

bool try_expand_vmem_bottom(VMemHandle handle, uint64_t size){
	return _try_expand_vmem_bottom(handle, size, true);
}

uint64_t get_vmem_size(VMemHandle handle){
	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;
	return get_no_padding_size(desc);
}
void* get_vmem_addr(VMemHandle handle){
	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;
	return get_no_padding_start_addr(desc);
}

VirtualMemoryType get_vmem_type(VMemHandle handle){
	VirtualMemoryDescriptor* desc = (VirtualMemoryDescriptor*)handle;
	return desc->type;
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
		if(old_prev != NULL)
			old_prev->next = descriptor;

		descriptor->prev = old_prev;
		descriptor->next = new_descriptor;
		new_descriptor->next = old_next;
		new_descriptor->prev = descriptor;

		if(old_next != NULL)
			old_next->prev = new_descriptor;

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

		if(old_prev != NULL)
			old_prev->next = prev_descriptor;

		prev_descriptor->prev = old_prev;
		prev_descriptor->next = mid_descriptor;
		mid_descriptor->prev = prev_descriptor;
		mid_descriptor->next = next_descriptor;
		next_descriptor->prev = mid_descriptor;
		next_descriptor->next = old_next;

		if(old_next != NULL)
			old_next->prev = next_descriptor;

		prev_descriptor->start_vaddr = old_start_vaddr;
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
	KASSERT(descriptor->size_bytes >= size)
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
	if(old_prev != NULL)
		old_prev->next = descriptor;

	descriptor->prev = old_prev;
	descriptor->next = new_descriptor;
	new_descriptor->next = old_next;
	new_descriptor->prev = descriptor;

	if(old_next != NULL)
		old_next->prev = new_descriptor;
	KASSERT(descriptor->start_vaddr+descriptor->size_bytes == new_descriptor->start_vaddr);
	update_head();

	return descriptor;
}

static void update_head(){
	while(kernel_vmm.vm_list_head->prev != NULL)
		kernel_vmm.vm_list_head = kernel_vmm.vm_list_head->prev;
}

static uint64_t get_no_padding_size(const VirtualMemoryDescriptor *d){
	return d->size_bytes - d->upper_padding - d->lower_padding;
	
}
static void* get_no_padding_start_addr(const VirtualMemoryDescriptor *d){
	return d->start_vaddr + d->lower_padding;
}

VMemHandle get_vmem_from_address(void* addr){
	VirtualMemoryDescriptor *descriptor = kernel_vmm.vm_list_head;
	uint64_t iaddr = (uint64_t)addr;

	while(descriptor->next != NULL){
		if(iaddr >= (uint64_t)descriptor->start_vaddr && iaddr < (uint64_t)(descriptor->start_vaddr)+(uint64_t)(descriptor->size_bytes))
			break;
		descriptor = descriptor->next;
	}
	if(addr >= descriptor->start_vaddr + descriptor->size_bytes)
		return NULL;
	if(addr < descriptor->start_vaddr)
		return NULL;
	return descriptor;
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

//TODO: make debug print thread safe
void debug_print_kernel_vmm(){
	print("\n-----kernel VMM state start-----\n");
	print("kernel paging structure: ");
	print_uint64_hex((uint64_t)kernel_vmm.kernel_paging_structure);
	print("\n");
	print("virtual adresses:\n");

	for(VirtualMemoryDescriptor* d = kernel_vmm.vm_list_head; d != NULL; d = d->next){
		print_uint64_hex((uint64_t)d->start_vaddr);
		print(" / ");
		print_uint64_hex((uint64_t)d->start_vaddr + d->size_bytes);
		print(" -> ");
		print(get_vm_type_string(d->type));
		print(" [");
		print_uint64_dec(get_no_padding_size(d)/MB);
		print(" MiB]");
		print("\n");
	}
	print("-----kernel VMM state end-----\n");
}

void print_page_fault_error(PageFaultInfo page_fault_info){
	print("[PAGE FAULT] ");
	uint64_t error = page_fault_info.page_error;

	if(error & 1<<1)
		print("(write) ");
	else
		print("(read) ");

	bool page_not_present = !(error & 1);
	if(page_not_present)
		print("page not present ");
	else if(error & 1<<3)
		print("reserved write ");
	if(error & 1 << 4)
		print("caused by instruction fetch ");
	if(error & 1 <<5)
		print("caused by Protection Key ");
	if(error & 1 <<6)
		print("caused by Shadow Stack Access ");
	if(error & 1 <<15)
		print("caused by SGX ");
	print("\n");

	print("[PAGE FAULT ADDRESS] ");
	print_uint64_hex((uint64_t)page_fault_info.fault_address);
	print("\n");
	if(page_fault_info.fault_address <= NULL + PAGE_SIZE)
		print("[PAGE FAULT] NULL PAGE POINTER DEREFERENCE\n");

	print("[PAGE FAULT VMEM] ");
	if(page_fault_info.fault_vmem == NULL){
		print("NULL vmem (not allocated virtual memory)");
	}else{
		print(get_vm_type_string(get_vmem_type(page_fault_info.fault_vmem)));
		print(" ");
		print_uint64_hex((uint64_t)get_vmem_addr(page_fault_info.fault_vmem));
		print(" -> ");
		print_uint64_hex((uint64_t)get_vmem_addr(page_fault_info.fault_vmem) + get_vmem_size(page_fault_info.fault_vmem));
	}
	print("\n");
}

//return true if the page fault is solved
bool page_fault_check_identity_map_load(PageFaultInfo pf_info){
	if(pf_info.fault_vmem == NULL)
		return false;

	VirtualMemoryType type = get_vmem_type(pf_info.fault_vmem);
	if(type == VM_TYPE_IDENTITY_MAP && pf_info.page_not_present){
#ifdef PRINT_ALL_PAGE_FAULTS
		if(is_kio_initialized()){
			print_page_fault_error(pf_info);
		}
#endif
		//load the identity map pages
		void* to_map = (void*)pf_info.fault_address - PAGE_FAULT_ADDITIONAL_PAGE_LOAD/2;
		void* last_page = to_map + (1+PAGE_FAULT_ADDITIONAL_PAGE_LOAD)*PAGE_SIZE;
		for(; to_map < last_page; to_map+=PAGE_SIZE){
			paging_map(kernel_vmm.kernel_paging_structure, to_map, to_map, PAGE_WRITABLE | PAGE_PRESENT);
		}
		VMMCacheShootdownRange shootdown_range;
		shootdown_range.addr_start = to_map;
		shootdown_range.size_in_pages = (last_page-to_map)/PAGE_SIZE;
		if(is_vmmcache_shootdown_subsystem_initialized())
			vmmcache_shootdown(shootdown_range);
		
		return true;
	}
	return false;
}

//return true if the page fault is solved
bool page_fault_check_stack_overflow(PageFaultInfo pf_info){
	if(pf_info.fault_vmem == NULL)
		return false;

	if(get_vmem_type(pf_info.fault_vmem) == VM_TYPE_STACK && pf_info.fault_address < get_vmem_addr(pf_info.fault_vmem) && pf_info.page_not_present){
		if(!is_kio_initialized())
			kpanic(UNRECOVERABLE_PAGE_FAULT);
		print_page_fault_error(pf_info);
		print("[PAGE FAULT] POSSIBLE STACK OVERFLOW\n");
		//possible stack overflow, try to expand
		//only if we do not consume the security minimum stack padding
		//used to check for stack overflows
		//TODO: add the get_lower_padding and get_upper_padding function and
		//use it in this function
		VirtualMemoryDescriptor* desc = pf_info.fault_vmem;
		int64_t remaining_padding = (int64_t)desc->lower_padding - KERNEL_STACK_EXPANSION_STEP;
		if(remaining_padding >= MINIMUM_STACK_PADDING){
			if(_try_expand_vmem_bottom(pf_info.fault_vmem, KERNEL_STACK_EXPANSION_STEP, false)){
				print("[PAGE FAULT] STACK EXPANDED, TRYING TO CONTINUE\n\n");
				return true;
			}
		}
		print("[PAGE FAULT] FAILED TO EXPAND THE STACK\n\n");
	}
	return false;
}

struct PageFaultState{
	bool servicing_page_fault;
	spinlock lock;
} page_fault_state = {false, {0}};

void page_fault(InterruptInfo info){
	uint64_t fault_address;
	asm("mov %%cr2, %0" : "=r"(fault_address) : : "cc");

#ifdef FREEZE_ON_PAGE_FAULT
	freeze_cpu();
#endif
#ifdef PRINT_ALL_PAGE_FAULTS
	if(is_kio_initialized())
		print_interrupt_savedcontext(info.saved_context);
#endif

	acquire_spinlock(&page_fault_state.lock);
	if(page_fault_state.servicing_page_fault){
		printf("[UNRECOVERABLE PAGE FAULT] fault addr=%h64", fault_address);
		kpanic_with_msg(UNRECOVERABLE_PAGE_FAULT, "double page fault detected");
	}
	page_fault_state.servicing_page_fault = true;
	release_spinlock(&page_fault_state.lock);
	
	//the interrupts are already disabled, no need for an hard lock
	acquire_spinlock(&kernel_vmm.lock);

	PageFaultInfo page_fault_info;
	page_fault_info.interrupt_info = info;
	page_fault_info.page_error = info.error;
	page_fault_info.fault_address = (void*)fault_address;
	page_fault_info.fault_vmem = get_vmem_from_address((void*)fault_address);
	page_fault_info.page_not_present = !(info.error & 1);

	bool page_fault_solved = page_fault_check_identity_map_load(page_fault_info);
	if(!page_fault_solved)
		page_fault_solved = page_fault_check_stack_overflow(page_fault_info);
		
	if(!page_fault_solved){
		print_page_fault_error(page_fault_info);
		printf("[PAGE FAULT] CANNOT RECOVER THE PAGE FAULT\n");
		kpanic(UNRECOVERABLE_PAGE_FAULT);
	}

	release_spinlock(&kernel_vmm.lock);

	acquire_spinlock(&page_fault_state.lock);
	page_fault_state.servicing_page_fault = false;
	release_spinlock(&page_fault_state.lock);

	return;
}

