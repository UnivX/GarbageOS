#include "memory.h"
#include "../hal.h"
#include "frame_allocator.h"

void memset(volatile void* dst, uint8_t data, size_t size){
	for(size_t i = 0; i < size; i++)
		((volatile uint8_t*)dst)[i] = data;
}

void memcpy(void* dst, const void* src, size_t size){
	for(size_t i = 0; i < size; i++)
		((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
}


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
