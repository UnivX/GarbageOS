#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "frame_allocator.h"
#include "HAL/bios/bootloader_data.h"

uint64_t kmain(){
	//test
	init_frame_allocator();
	//set_up_arch_layer();
	return get_bootloader_data()->vbe_mode_info->bpp;
}
