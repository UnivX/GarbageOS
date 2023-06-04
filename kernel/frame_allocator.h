#pragma once
#include <stdbool.h>
#include "hal.h"

struct FrameAllocatorState{
	bool is_initialized;
	FreePhysicalMemoryStruct free_memory;
};

bool is_frame_allocator_initialized();
void init_frame_allocator();
void* alloc_frame();
