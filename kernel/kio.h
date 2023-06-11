#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "display_interface.h"
#include "kdefs.h"
#include "hal.h"
#include "psf.h"
#define BUFFER_SIZE 1024*768

typedef struct KioState{
	DisplayInterface display;
	uint64_t next_x, next_y;
	Pixel background_color, font_color;
	PSFFont font;
	Pixel buffer[BUFFER_SIZE];
} KioState;

//the display interface needs to be initialized
//the display will be finalized during the
//finalize_kio() function
void init_kio(DisplayInterface display, PSFFont font, Pixel background_color, Pixel font_color);
void putchar(char c, bool flush);
void print(char* str);
void finalize_kio();
