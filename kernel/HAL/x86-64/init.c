#include "cpu.h"

static bool is_initialized = false;

void set_up_caching(){
	asm volatile(
			"mov %%cr0, %%rax\n"
			"and $~(1<<29), %%rax\n"
			"and $~(1<<30), %%rax\n"
			"mov %%rax, %%cr0\n"
			: : : "cc", "rax");
}

void init_cpu(){
	set_up_caching();
	make_CPU_states();
	load_CPU_state();
}

void early_set_up_arch_layer(){
	//do nothing :)
}

void set_up_arch_layer(){
	init_cpu();
	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
