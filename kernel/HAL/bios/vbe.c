#include <stddef.h>
#include "vbe.h"
#include "../../memory.h"
#include "../../hal.h"
#include "../../kdefs.h"

VbeFrameBuffer init_frame_buffer(void* virtual_address, uint64_t max_size_bytes){
	if(virtual_address == NULL)
		kpanic(VBE_ERROR);

	VbeFrameBuffer invalid_buffer = {NULL, NULL, NULL};
	VbeFrameBuffer framebuffer;
	framebuffer.vbe_mode_info =  get_bootloader_data()->vbe_mode_info;
	framebuffer.vaddr = virtual_address;
	framebuffer.paddr = (void*)((uint64_t)framebuffer.vbe_mode_info->framebuffer);

	uint64_t bytes_count = ((framebuffer.vbe_mode_info->bpp/8)*framebuffer.vbe_mode_info->height*framebuffer.vbe_mode_info->width);
	uint64_t pages_count = bytes_count/PAGE_SIZE + (bytes_count%PAGE_SIZE != 0);//round up

	if(bytes_count > max_size_bytes)
		return invalid_buffer;
																				
	for(uint64_t i = 0; i < pages_count; i++){
		void* vaddr_to_map = (void*)(framebuffer.vaddr+(i*PAGE_SIZE));
		void* paddr_to_map = (void*)(framebuffer.paddr+(i*PAGE_SIZE));
		mmap(vaddr_to_map, paddr_to_map, PAGE_WRITABLE);
	}
	return framebuffer;
}

bool is_frame_buffer_valid(VbeFrameBuffer framebuffer){
	return !(framebuffer.paddr == NULL || framebuffer.vaddr == NULL || framebuffer.vbe_mode_info == NULL);
}

void fill_screen(VbeFrameBuffer framebuffer, RGBAPixel color){
	if(!is_frame_buffer_valid(framebuffer))
		kpanic(VBE_ERROR);

	uint64_t pixel_count = framebuffer.vbe_mode_info->height*framebuffer.vbe_mode_info->width;
	if(framebuffer.vbe_mode_info->bpp == 24){
		//rgb encoding 8 bit each
		volatile uint8_t* dest_addr = (volatile uint8_t*)framebuffer.vaddr;
		for(uint64_t i = 0; i < pixel_count; i++){
			*dest_addr = color.b;
			dest_addr++;
			*dest_addr = color.g;
			dest_addr++;
			*dest_addr = color.r;
			dest_addr++;
		}
	}else if(framebuffer.vbe_mode_info->bpp == 32){
		//rgba encoding 8 bit each
		volatile uint8_t* dest_addr = (volatile uint8_t*)framebuffer.vaddr;
		for(uint64_t i = 0; i < pixel_count; i++){
			*dest_addr = color.b;
			dest_addr++;
			*dest_addr = color.g;
			dest_addr++;
			*dest_addr = color.r;
			dest_addr++;
			*dest_addr = color.a;
			dest_addr++;
		}
	}else{
		kpanic(VBE_ERROR);
	}
	return;
}
