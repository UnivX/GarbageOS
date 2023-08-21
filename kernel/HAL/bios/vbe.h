#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "bootloader_data.h"
#include "../../mem/vmm.h"

typedef struct VbeFrameBuffer{
	void* vaddr;
	void* paddr;
	VbeModeInfoStructure* vbe_mode_info;
	VMemHandle framebuffer_mapping;
} VbeFrameBuffer;

typedef struct RGBAPixel{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} __attribute__ ((packed)) RGBAPixel;

/*if the size of the framebuffer is over the specified max_size_bytes of virtual mem space then it will return an invalid object*/
VbeFrameBuffer init_frame_buffer();
void destroy_frame_buffer(VbeFrameBuffer framebuffer);
void fill_screen(VbeFrameBuffer framebuffer, RGBAPixel color);
bool is_frame_buffer_valid(VbeFrameBuffer framebuffer);
