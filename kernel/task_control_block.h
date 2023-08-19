#pragma once
#include "mem/vmm.h"
#define TSB_SUPERVISOR
#define TSB_KERNEL

typedef struct TaskControlBlock{
	uint64_t id;
	uint64_t flags;
	VirtualMemoryManager* vmm;
	void* paging_structure;
} TaskControlBlock;

TaskControlBlock* get_task_control_block();
