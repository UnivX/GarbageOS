#include "frame_allocator.h"
#include "kdefs.h"
#include "hal.h"

struct FrameAllocatorState frame_allocator={false, {0, NULL}, {NULL, NULL, 0}};

bool is_frame_allocator_initialized(){
	return frame_allocator.is_initialized;
}

void init_frame_allocator(){
	frame_allocator.is_initialized=true;
	frame_allocator.free_memory = free_mem_bootloader();
	
	/* the frame allocator needs to be usable in a pre-inizialization envioroment so all the data must be
	 * inside the bootstage identity mapped memory
	 */
	PhysicalMemoryRange bootstage_usable = get_bootstage_indentity_mapped_RAM();
	uint64_t first_bootstage_unusable_addr = bootstage_usable.start_address + bootstage_usable.size;
	uint64_t usable_ram_size = get_total_usable_RAM_size();
	uint64_t needed_bytes_stack_frame = usable_ram_size / PAGE_SIZE* sizeof(uint64_t);
	needed_bytes_stack_frame += PAGE_SIZE;//an extra page just to be sure
	//round up to PAGE_SIZE
	needed_bytes_stack_frame += -(needed_bytes_stack_frame%PAGE_SIZE)+PAGE_SIZE;

	
	bool range_found = false;
	PhysicalMemoryRange* free_ranges = frame_allocator.free_memory.free_ranges;
	for(size_t i = 0; i < frame_allocator.free_memory.number_of_ranges && !range_found; i++){
		bool is_usable_in_bootstage = free_ranges[i].start_address + needed_bytes_stack_frame < first_bootstage_unusable_addr &&
										free_ranges[i].start_address >= bootstage_usable.start_address;
		bool is_large_enough = free_ranges[i].size >= needed_bytes_stack_frame;
		if(is_large_enough && is_usable_in_bootstage){
			range_found = true;
			frame_allocator.frame_stack = make_frame_stack((void*)free_ranges[i].start_address, needed_bytes_stack_frame);
			free_ranges[i].start_address += needed_bytes_stack_frame;
			free_ranges[i].size -= needed_bytes_stack_frame;
		}
	}

	//without frame stack is impossible to continue
	if(!range_found)
		kpanic(FRAME_ALLOCATOR_ERROR);
}

void* alloc_frame(){
	if(is_hal_arch_initialized()){

	}else{
		kpanic(FRAME_ALLOCATOR_ERROR);
	}
	kpanic(FRAME_ALLOCATOR_ERROR);
	return NULL;
}

FrameStack make_frame_stack(void* addr, uint64_t size){
	FrameStack stack;
	stack.base = addr;
	stack.top = addr;
	stack.max_size = size;
	return stack;
}
bool is_frame_stack_valid(FrameStack stack){
	return stack.max_size != 0 && stack.base != NULL && stack.top != NULL && stack.top >= stack.base;
}

uint64_t frame_stack_size(FrameStack stack){
	return (uint64_t)stack.top - (uint64_t)stack.base;
}
