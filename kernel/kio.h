#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include "display_interface.h"
#include "util/sync_types.h"
#include "kdefs.h"
#include "hal.h"
#include "psf.h"

typedef struct KioState{
	DisplayInterface display;
	uint64_t next_x, next_y;
	uint64_t screen_size;
	Color background_color, font_color;
	PSFFont font;
	Pixel *buffer;
	spinlock lock;
} KioState;

//the display interface needs to be initialized
//the display will be finalized during the
//finalize_kio() function
bool is_kio_initialized();
void init_kio(const DisplayInterface display, const PSFFont font, const Color background_color, const Color font_color);
void putchar(char c, bool flush);
void print(const char* str);
//%u64
//%h64
//%s
void printf(const char* format, ...);//TODO: test
void print_uint64_hex(uint64_t n);
void print_uint64_dec(uint64_t n);
void finalize_kio();
