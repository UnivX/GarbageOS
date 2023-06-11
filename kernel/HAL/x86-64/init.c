#include "../../hal.h"
#include "x64-memory.h"
static bool is_initialized = false;

void set_up_arch_layer(){
	identity_map_memory();
	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
