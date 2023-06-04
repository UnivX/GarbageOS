#pragma once
#include <stdint.h>
#include <stddef.h>

#define MAX_PHYSICAL_MEMORY_RANGES 128
#define MEMORY_MAP_FREE 1
#define MIN_BOOT_FRAME_ALLOC_ADDR 0x01000000

//frame allocator data from the bootloader
typedef struct FrameData{
	uint32_t memory_map_item_offset;
	uint32_t first_frame_address;
}__attribute__((packed)) FrameData;

//single item result of the call of the BIOS function INT 0x15, EAX = 0xE820
//done by the bootloader
typedef struct MemoryMapItem{
	uint64_t base_addr;
	uint64_t size;
	uint32_t type;
	uint32_t acpi;
}__attribute__((packed)) MemoryMapItem;

typedef struct VbeModeInfoStructure {
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
} __attribute__ ((packed)) VbeModeInfoStructure;


typedef struct BootLoaderData{
	uint64_t magic_number;
	VbeModeInfoStructure* vbe_mode_info;
	FrameData* frame_allocator_data;
	uint32_t* map_items_count;
	MemoryMapItem* map_items;
} __attribute__ ((packed)) BootLoaderData;

BootLoaderData* get_bootloader_data();
