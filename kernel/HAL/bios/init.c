#include "vbe.h"
#include "../../kdefs.h"

void set_up_firmware_layer(){
	VbeFrameBuffer framebuffer = init_frame_buffer((void*)0xffff800000000000, 1*GB);
	RGBAPixel blue={0,0,255,255};
	fill_screen(framebuffer, blue);
}
