#include <stdint.h>
#include <stddef.h>
#include "psf.h"
#include "kdefs.h"
#include "hal.h"
#include "frame_allocator.h"
#include "display_interface.h"
#define BUFFER_SIZE 1024*768
Pixel buffer[BUFFER_SIZE];

uint64_t kmain(){
	//test
	init_frame_allocator();
	set_up_firmware_layer();
	set_up_arch_layer();

	DisplayInterface display = get_firmware_display();
	display.init();
	if(display.info.height*display.info.width > BUFFER_SIZE)
		return -1;

	Vector2i buffer_size = {display.info.width, display.info.height};

	Pixel background_color = {0,0,0,255};
	Pixel font_color = {255,255,255,255};

	for(int i = 0; i < BUFFER_SIZE; i++){
		buffer[i] = background_color;
	}

	PSFFont font = get_default_PSF_font();
	Vector2i pos = {0, 0};

	write_PSF_char(font, 'Q', pos, buffer, buffer_size, background_color, font_color);
	display.write_pixels(buffer, BUFFER_SIZE);
	return font.is_valid;
}
