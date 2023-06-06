#pragma once
#include <stdbool.h>
#include "hal.h"

/* a stack that requires a contiguous physical ram space
 * used to take care of freed frames
 */
typedef struct FrameStack{
	uint64_t* base;//points to the base of the stack
	uint64_t* top;//points past the last element of the stack
	uint64_t max_size;
} FrameStack;

FrameStack make_frame_stack(void* addr, uint64_t size);
bool is_frame_stack_valid(FrameStack stack);
uint64_t frame_stack_size(FrameStack stack);

/* the frame allocator needs to be usable in a pre-inizialization envioroment */
struct FrameAllocatorState{
	bool is_initialized;
	FreePhysicalMemoryStruct free_memory;
	FrameStack frame_stack;
};


bool is_frame_allocator_initialized();
void init_frame_allocator();
void* alloc_frame();
