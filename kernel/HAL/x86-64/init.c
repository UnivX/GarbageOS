#include "../../hal.h"
#include "x64-memory.h"
#include "gdt.h"
static bool is_initialized = false;

GDT gdt[5];
void set_up_arch_layer(){
	identity_map_memory();
	//null descriptor
	gdt[0] = make_gdt(0,0,0,0);
	//kernel code descriptor
	gdt[1] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(0) | NOT_SYSTEM | EXECUTABLE | CONFORMING | READ_WRITE);
	//kernel data descriptor
	gdt[2] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(0) | NOT_SYSTEM | READ_WRITE);


	//user code descriptor
	gdt[3] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(3) | NOT_SYSTEM | EXECUTABLE | READ_WRITE);
	//user data descriptor
	gdt[4] = make_gdt(0, 0xffffff, GRANULARITY_FLAG | LONG_MODE_FLAG,
			PRESENT | DPL(3) | NOT_SYSTEM | READ_WRITE);

	load_gdt(gdt, 5);

	is_initialized = true;
}
bool is_hal_arch_initialized(){
	return is_initialized;
}
