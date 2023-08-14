#include "vmm.h"
#include "../hal.h"
#include "frame_allocator.h"

void* next_free_stack_vaddr = KERNEL_ADDITIONAL_STACKS_VADDR;
//return the stack top pointer
void* alloc_stack(uint64_t size){
	KASSERT(next_free_stack_vaddr - KERNEL_ADDITIONAL_STACKS_VADDR + size <= KERNEL_ADDITIONAL_STACK_SIZE);
	size += PAGE_SIZE - (size % PAGE_SIZE);

	for(uint64_t i = size; i != 0; i-=PAGE_SIZE){
		mmap(next_free_stack_vaddr, alloc_frame(), PAGE_WRITABLE | PAGE_PRESENT);
		next_free_stack_vaddr += PAGE_SIZE;
	}
	return next_free_stack_vaddr;
}

bool heap_allocated = false;
void* alloc_kernel_heap(){
	KASSERT(!heap_allocated);
	heap_allocated = true;

	KASSERT(KERNEL_HEAP_SIZE % PAGE_SIZE == 0);
	KASSERT((uint64_t)KERNEL_HEAP_VADDR % PAGE_SIZE == 0);

	void* next_heap_addr = KERNEL_HEAP_VADDR;
	for(uint64_t i = KERNEL_HEAP_SIZE; i != 0; i-=PAGE_SIZE){
		mmap(next_heap_addr, alloc_frame(), PAGE_WRITABLE | PAGE_PRESENT);
		next_heap_addr += PAGE_SIZE;
	}
	return KERNEL_HEAP_VADDR;
}
