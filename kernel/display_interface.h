#pragma once
#include <stdint.h>

typedef struct Pixel{
	uint8_t r,g,b,a;
} Pixel;

typedef enum DisplayType{
	VBE_DISPLAY
} DisplayType;

typedef struct DisplayInfo{
	uint32_t width, height;
	uint32_t depth;//in bits
	DisplayType type;
} DisplayInfo;

typedef struct DisplayInterface{
	DisplayInfo info;
	//initialize the display
	void(*init)(void);
	//finalize the display
	void(*finalize)(void);
	//write to screen the pixel buffer
	void(*write_pixels)(Pixel pixels[], uint64_t size);
	
} DisplayInterface;
