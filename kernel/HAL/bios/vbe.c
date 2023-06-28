#include <stddef.h>
#include "vbe.h"
#include "../../memory.h"
#include "../../hal.h"
#include "../../kdefs.h"

VbeFrameBuffer global_frame_buffer = {NULL, NULL, NULL};

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

void init_vbe_display(){
	global_frame_buffer = init_frame_buffer(VBE_VADDR, VBE_VADDR_SIZE);
}

void finalize_vbe_display(){
	//nothing to clear
}

void write_pixels_vbe_display(Pixel pixels[], uint64_t size){
	if(!is_frame_buffer_valid(global_frame_buffer))
		kpanic(VBE_ERROR);

	uint64_t pixel_count = global_frame_buffer.vbe_mode_info->height*global_frame_buffer.vbe_mode_info->width;

	if(size > pixel_count)
		kpanic(VBE_ERROR);

	if(global_frame_buffer.vbe_mode_info->bpp == 24){
		//rgb encoding 8 bit each
		volatile uint8_t* dest_addr = (volatile uint8_t*)global_frame_buffer.vaddr;
		for(uint64_t i = 0; i < size; i++){
			*dest_addr = pixels[i].b;
			dest_addr++;
			*dest_addr = pixels[i].g;
			dest_addr++;
			*dest_addr = pixels[i].r;
			dest_addr++;
		}
	}else if(global_frame_buffer.vbe_mode_info->bpp == 32){
		//rgba encoding 8 bit each
		/*
		volatile uint8_t* dest_addr = (volatile uint8_t*)global_frame_buffer.vaddr;
		for(uint64_t i = 0; i < pixel_count; i++){
			*dest_addr = pixels[i].b;
			dest_addr++;
			*dest_addr = pixels[i].g;
			dest_addr++;
			*dest_addr = pixels[i].r;
			dest_addr++;
			*dest_addr = pixels[i].a;
			dest_addr++;
		}
		*/
		volatile Pixel* dest_addr = (volatile Pixel*)global_frame_buffer.vaddr;
		for(int i = 0; i < pixel_count; i++){
			dest_addr[i] = pixels[i];
		}
	}else{
		kpanic(VBE_ERROR);
	}
}

DisplayInterface get_firmware_display(){
	VbeModeInfoStructure* vbe_mode_info = get_bootloader_data()->vbe_mode_info;
	DisplayInfo display_info = {vbe_mode_info->width, vbe_mode_info->height, vbe_mode_info->bpp, VBE_DISPLAY};
	DisplayInterface display_interface = {display_info, init_vbe_display,
		finalize_vbe_display, write_pixels_vbe_display};
	return display_interface;
}
