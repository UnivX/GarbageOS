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
#include "util/circular_buffer.h"
#define KIO_CIRCULAR_BUFFER_SIZE 4096

//TODO remove the hard spinlock and use a soft one
//possible solution to reduce the locking: store the printed data in the circular buffer when in interrupt, unless kio_flush is called
typedef struct KioState{
	CircularBuffer char_buffer;
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
void kio_flush();
//u = unsigned decimal print
//x = hexadecimal print
//h = padded hexadecimal print
//%u/x/h64
//%u/x/h32
//%s
void printf(const char* format, ...);
void print_uint64_hex(uint64_t n);
void print_uint64_dec(uint64_t n);
void finalize_kio();
