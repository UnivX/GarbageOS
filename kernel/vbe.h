#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct vbe_mode_info_structure {
	uint16_t attributes;
	uint8_t window_a;
	uint8_t window_b;
	uint16_t granularity;
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;
	uint16_t pitch;
	uint16_t width;
	uint16_t height;
	uint8_t w_char;
	uint8_t y_char;
	uint8_t planes;
	uint8_t bpp;
	uint8_t banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t image_pages;
	uint8_t reserved0;
 
	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;
 
	uint32_t framebuffer;
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;
	uint8_t reserved1[206];
} __attribute__ ((packed)) vbe_mode_info_structure;

typedef struct vbe_frame_buffer{
	void* vaddr;
	void* paddr;
	vbe_mode_info_structure* vbe_mode_info;
} vbe_frame_buffer;

typedef struct rbga_pixel{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} __attribute__ ((packed)) rbga_pixel;

/*if the size of the framebuffer is over the specified max_size_bytes of virtual mem space then it will return an invalid object*/
vbe_frame_buffer init_frame_buffer(void* virtual_address, uint64_t max_size_bytes);
void fill_screen(vbe_frame_buffer framebuffer, rbga_pixel color);
bool is_frame_buffer_valid(vbe_frame_buffer framebuffer);
