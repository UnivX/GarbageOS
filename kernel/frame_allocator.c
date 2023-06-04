#include "frame_allocator.h"
#include "kdefs.h"
#include "hal.h"

struct FrameAllocatorState frame_allocator={false, {0, NULL}};

bool is_frame_allocator_initialized(){
	return frame_allocator.is_initialized;
}

void init_frame_allocator(){
	frame_allocator.is_initialized=true;
	frame_allocator.free_memory = free_mem_bootloader();
}

void* alloc_frame(){
	if(!is_hal_arch_initialized()){
		kpanic(FRAME_ALLOCATOR_ERROR);
	}
	kpanic(FRAME_ALLOCATOR_ERROR);
	return NULL;
}

