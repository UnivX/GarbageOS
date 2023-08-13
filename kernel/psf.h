#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "display_interface.h"
#include "kdefs.h"
#include "hal.h"
#define PSF2_MAGIC_NUMBER 0x864ab572
#define ASCII_TABLE_SIZE 128

//embedded font
extern const uint8_t _binary_font_Tamsyn10x20r_psf_start[];
extern const uint8_t _binary_font_Tamsyn10x20r_psf_end[];

typedef struct PSFHeader{
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} __attribute__((packed)) PSFHeader;

typedef struct PSFFont {
	PSFHeader* header;//also the start of the file in memory
	uint16_t ascii_table[ASCII_TABLE_SIZE];
	bool is_valid;
	bool has_table;
} PSFFont;


PSFFont get_default_PSF_font();
bool write_PSF_char(PSFFont font, unsigned char c, Vector2i position, Pixel buffer[], Vector2i buffer_size,
		Color background_color, Color font_color);
