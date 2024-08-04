#include "frame_allocator.h"
#include "../kdefs.h"
#include "../hal.h"
#include "../kio.h"

//TODO: protect internal state from race conditions
struct FrameAllocatorState frame_allocator={false, {0, NULL}, {NULL, NULL, 0}, 0, 0};

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
			frame_allocator.cached_memory_range_index = i;//cache the memory range that is boostage friendly
			frame_allocator.frame_stack = make_frame_stack((void*)free_ranges[i].start_address, needed_bytes_stack_frame);
			free_ranges[i].start_address += needed_bytes_stack_frame;
			free_ranges[i].size -= needed_bytes_stack_frame;
		}
	}

	//calc the size
	for(size_t i = 0; i < frame_allocator.free_memory.number_of_ranges; i++)
		frame_allocator.free_frames += free_ranges[i].size / PAGE_SIZE;

	//without frame stack is impossible to continue
	if(!range_found)
		kpanic(FRAME_ALLOCATOR_ERROR);
}

void* alloc_frame(){
	if(!is_frame_stack_valid(frame_allocator.frame_stack))
		kpanic(FRAME_ALLOCATOR_ERROR);

	frame_allocator.free_frames--;

	//if there are frames in the frame_stack use one of them
	if(frame_stack_count(frame_allocator.frame_stack) > 0){
		return (void*)frame_stack_pop(&frame_allocator.frame_stack);
	}

	PhysicalMemoryRange* free_ranges = frame_allocator.free_memory.free_ranges;
	PhysicalMemoryRange bootstage_usable = get_bootstage_indentity_mapped_RAM();
	uint64_t first_bootstage_unusable_addr = bootstage_usable.start_address + bootstage_usable.size;
	
	bool is_cached_index_usable_bootstage = free_ranges[frame_allocator.cached_memory_range_index].start_address + PAGE_SIZE < first_bootstage_unusable_addr &&
										free_ranges[frame_allocator.cached_memory_range_index].start_address >= bootstage_usable.start_address;
	bool is_cached_index_big_enough = free_ranges[frame_allocator.cached_memory_range_index].size >= PAGE_SIZE;

	//if there are no frames to reuse take one from 
	//if we are in the bootstage we have a cached_memory_range_index that
	//it's usable in this stage
	if(is_hal_arch_initialized()){
		//if we are out of the bootstage then we need to check only if it's big enough
		if(is_cached_index_big_enough){
			void* new_frame = (void*)free_ranges[frame_allocator.cached_memory_range_index].start_address;
			free_ranges[frame_allocator.cached_memory_range_index].size-=PAGE_SIZE;
			free_ranges[frame_allocator.cached_memory_range_index].start_address+=PAGE_SIZE;
			return new_frame;
		}else{
			//search for a new range
			bool range_found = false;
			for(size_t i = 0; i < frame_allocator.free_memory.number_of_ranges && !range_found; i++){
				if(free_ranges[i].size >= PAGE_SIZE){
					range_found = true;
					frame_allocator.cached_memory_range_index = i;//cache the memory range that is boostage friendly
				}
			}
			if(!range_found)
				kpanic(FRAME_ALLOCATOR_ERROR);

			void* new_frame = (void*)free_ranges[frame_allocator.cached_memory_range_index].start_address;
			free_ranges[frame_allocator.cached_memory_range_index].size-=PAGE_SIZE;
			free_ranges[frame_allocator.cached_memory_range_index].start_address+=PAGE_SIZE;
			return new_frame;
		}
	}else{
		//if we are in the boostage then we need to check if the range is usable in the bootstage
		if(is_cached_index_big_enough && is_cached_index_usable_bootstage){
			void* new_frame = (void*)free_ranges[frame_allocator.cached_memory_range_index].start_address;
			free_ranges[frame_allocator.cached_memory_range_index].size-=PAGE_SIZE;
			free_ranges[frame_allocator.cached_memory_range_index].start_address+=PAGE_SIZE;
			return new_frame;
		}else{
			//search for a new range
			bool range_found = false;
			for(size_t i = 0; i < frame_allocator.free_memory.number_of_ranges && !range_found; i++){
				bool is_usable_in_bootstage = free_ranges[i].start_address + PAGE_SIZE < first_bootstage_unusable_addr &&
											free_ranges[i].start_address >= bootstage_usable.start_address;
				bool is_large_enough = free_ranges[i].size >= PAGE_SIZE;
				if(is_large_enough && is_usable_in_bootstage){
					range_found = true;
					frame_allocator.cached_memory_range_index = i;//cache the memory range that is boostage friendly
				}
			}
			if(!range_found)
				kpanic(FRAME_ALLOCATOR_ERROR);

			void* new_frame = (void*)free_ranges[frame_allocator.cached_memory_range_index].start_address;
			free_ranges[frame_allocator.cached_memory_range_index].size-=PAGE_SIZE;
			free_ranges[frame_allocator.cached_memory_range_index].start_address+=PAGE_SIZE;
			return new_frame;
		}
	}
	kpanic(FRAME_ALLOCATOR_ERROR);
	return NULL;
}

void dealloc_frame(void* paddr){
	frame_allocator.free_frames++;
	frame_stack_push(&frame_allocator.frame_stack, (uint64_t)paddr);
}

FrameStack make_frame_stack(void* addr, uint64_t size){
	FrameStack stack;
	stack.base = addr;
	stack.top = addr;
	stack.max_size_bytes = size;
	return stack;
}
bool is_frame_stack_valid(FrameStack stack){
	return stack.max_size_bytes != 0 && stack.base != NULL && stack.top != NULL && stack.top >= stack.base;
}

uint64_t frame_stack_size_bytes(FrameStack stack){
	return (uint64_t)stack.top - (uint64_t)stack.base;
}

uint64_t frame_stack_count(FrameStack stack){
	return frame_stack_size_bytes(stack) / sizeof(uint64_t);
}

uint64_t frame_stack_pop(FrameStack* stack){
	if(frame_stack_count(*stack) <= 0)
		kpanic(FRAME_ALLOCATOR_ERROR);

	stack->top--;
	return *stack->top;
}
void frame_stack_push(FrameStack* stack, uint64_t new_value){
	if(frame_stack_size_bytes(*stack)+sizeof(uint64_t) > stack->max_size_bytes)
		kpanic(FRAME_ALLOCATOR_ERROR);
	*stack->top = new_value;
	stack->top++;
}
uint64_t get_number_of_free_frames(){
	return frame_allocator.free_frames;
}

uint64_t get_frame_allocator_mem_overhead(){
	if(!is_frame_stack_valid(frame_allocator.frame_stack))
		kpanic(FRAME_ALLOCATOR_ERROR);
	return frame_allocator.frame_stack.max_size_bytes;
}
