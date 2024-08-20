#pragma once
#include <stdbool.h>
#include "../hal.h"
#include "../util/sync_types.h"

/* a stack that requires a contiguous physical ram space
 * used to take care of freed frames
 */
typedef struct FrameStack{
	uint64_t* base;//points to the base of the stack
	uint64_t* top;//points past the last element of the stack
	uint64_t max_size_bytes;
} FrameStack;

FrameStack make_frame_stack(void* addr, uint64_t size);
bool is_frame_stack_valid(FrameStack stack);
uint64_t frame_stack_size_bytes(FrameStack stack);
uint64_t frame_stack_count(FrameStack stack);
uint64_t frame_stack_pop(FrameStack* stack);
void frame_stack_push(FrameStack* stack, uint64_t new_value);

/* the frame allocator needs to be usable in a pre-inizialization envioroment */
struct FrameAllocatorState{
	bool is_initialized;
	FreePhysicalMemoryStruct free_memory;
	FrameStack frame_stack;
	size_t cached_memory_range_index;//mem range used by the allocate
	uint64_t free_frames;
	spinlock lock;
};


bool is_frame_allocator_initialized();
void init_frame_allocator();

void* alloc_frame();
void dealloc_frame(void* paddr);
uint64_t get_number_of_free_frames();
uint64_t get_frame_allocator_mem_overhead();
