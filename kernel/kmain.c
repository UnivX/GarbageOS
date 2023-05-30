#include <stdint.h>
#include <stddef.h>
#include "vbe.h"
#include "misc.h"

uint64_t kmain(){
	//test
	vbe_frame_buffer framebuffer = init_frame_buffer((void*)0xffff800000000000, 100*MB);
	rbga_pixel blue = {0,0,255,255};
	fill_screen(framebuffer, blue);
	return 0;
}
