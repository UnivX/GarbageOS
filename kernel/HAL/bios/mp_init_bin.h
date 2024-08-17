#pragma once
#include <stdint.h>


typedef void(*APEntryPoint)(void*);

typedef struct MPInitializationData{
	void** stacks_top;
	uint64_t APs_count;
	//the ap_entry_point takes as argument the stack top pointer;
	APEntryPoint ap_entry_point;
}__attribute__((packed)) MPInitializationData;

volatile void* create_mp_init_routine_in_RAM(const void* paging_structure, MPInitializationData* init_data);
