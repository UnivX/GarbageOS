#pragma once
#include <stdint.h>

typedef struct Pixel{
	uint8_t b,g,r,a;
} __attribute__((packed)) Pixel;


typedef struct Color{
	uint8_t r,g,b,a;
} __attribute__((packed)) Color;

Pixel color_to_pixel(Color c);
Color pixel_to_color(Pixel p);

typedef enum DisplayType{
	VBE_DISPLAY
} DisplayType;

typedef struct DisplayInfo{
	uint32_t width, height;
	uint32_t depth;//in bits
	DisplayType type;
	void* implementation_data;
} DisplayInfo;

typedef struct DisplayInterface{
	DisplayInfo info;
	//initialize the display
	void(*init)(struct DisplayInterface interface);
	//finalize the display
	void(*finalize)(struct DisplayInterface interface);
	//write to screen the pixel buffer
	void(*write_pixels)(struct DisplayInterface interface, Pixel pixels[], uint64_t size);
	
} DisplayInterface;
