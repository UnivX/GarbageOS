/*
Virtual Memory Manager(VMM)
*/

#include <stdint.h>
#include <stddef.h>
#include "../kdefs.h"

//allocate a new kernel stack
//return the stack top pointer
void* alloc_stack(uint64_t size);
void* alloc_kernel_heap();
