#include <stdint.h>
#include <stddef.h>
#include "kdefs.h"
#include "hal.h"
#include "frame_allocator.h"
#include "kio.h"

uint64_t kmain(){
	//test
	init_frame_allocator();
	set_up_firmware_layer();
	set_up_arch_layer();

	DisplayInterface display = get_firmware_display();
	display.init();
	if(display.info.height*display.info.width > BUFFER_SIZE)
		return -1;

	Pixel background_color = {0,0,0,255};
	Pixel font_color = {0,255,0,255};
	PSFFont font = get_default_PSF_font();
	init_kio(display, font, background_color, font_color);

	print("Hello World\nI'm a kernel");

	return false;
}
