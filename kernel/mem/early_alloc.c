#include "early_alloc.h"

uint8_t early_allocator_mem[EARLY_ALLOCATOR_SIZE];
uint64_t free_size = EARLY_ALLOCATOR_SIZE;
void* next_free_mem = (void*)early_allocator_mem;

void* early_alloc(size_t size){
	if(size > free_size)
		kpanic(EARLY_ALLOCATOR_ERROR);
	void* allocated_mem = next_free_mem;
	next_free_mem += size;
	free_size -= size;
	return allocated_mem;
}
