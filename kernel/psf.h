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
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} __attribute__((packed)) PSFHeader;

typedef struct PSFFont {
	PSFHeader* header;//also the start of the file in memory
	uint16_t ascii_table[ASCII_TABLE_SIZE];
	bool is_valid;
	bool has_table;
} PSFFont;


PSFFont get_default_PSF_font();
bool write_PSF_char(PSFFont font, unsigned char c, Vector2i position, Pixel buffer[], Vector2i buffer_size,
		Pixel background_color, Pixel font_color);
