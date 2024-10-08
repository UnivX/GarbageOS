#include "psf.h"
#include "mem/heap.h"

void calc_glyphs_caches(PSFFont* font){
	int line_in_bytes = (font->header->width+7)/8;//divide by 8 and add 1 if there is a remainder
	font->glyphs = kmalloc(sizeof(CachedGlyph)*font->header->numglyph);
	uint64_t font_width = font->header->width;
	uint64_t font_height = font->header->height;

	for(uint64_t i = 0; i < font->header->numglyph; i++){
		font->glyphs[i].pixels = kmalloc(font_width*font_height*sizeof(bool));
		uint8_t *glyph = (uint8_t*)font->header+ font->header->headersize + i*font->header->bytesperglyph;
		for(uint32_t y = 0; y < font_height; y++){
			for(uint32_t x = 0; x < font_width; x++){
				KASSERT(x+y*font_width < font_width*font_height);
				bool is_font_pixel = (glyph[x/8] << (x%8))&0x80;
				font->glyphs[i].pixels[x+y*font_width] = is_font_pixel;
			}
			//next line to display
			glyph += line_in_bytes;
		}
	}
}

PSFFont get_default_PSF_font(){
	PSFFont invalid_font = {NULL, {}, false, false, NULL};

	PSFFont font;
	font.glyphs = NULL;
	font.header = (PSFHeader*)(void*)_binary_font_Tamsyn10x20r_psf_start;
	font.is_valid = true; if(font.header->magic != PSF2_MAGIC_NUMBER)
		return invalid_font;

	uint16_t glyph = 0;
	uint32_t bitmaps_size = (font.header->bytesperglyph*font.header->numglyph);
	uint8_t* s = ((uint8_t*)_binary_font_Tamsyn10x20r_psf_start) + bitmaps_size + font.header->headersize;
	
	for(uint16_t i = 0; i < ASCII_TABLE_SIZE; i++)
		font.ascii_table[i] = 0;

	//if it doesnt have a unicode table
	font.has_table = font.header->flags == 0;
	if(!font.has_table){
		for(uint16_t i = 0; i < ASCII_TABLE_SIZE; i++)
			font.ascii_table[i] = i;
		calc_glyphs_caches(&font);
		return font;
	}
	while(s<_binary_font_Tamsyn10x20r_psf_end) {
        uint16_t uc = (uint16_t)(((unsigned char *)s)[0]);
        if(uc == 0xFF) {
            glyph++;
            s++;
            continue;
        } else if(uc & 128) {
            /* UTF-8 to unicode */
            if((uc & 32) == 0 )
                s++;
            else if((uc & 16) == 0 )
                s+=2;
            else if((uc & 8) == 0 )
                s+=3;
            else
				uc = 0;
        }else{
			//if it's ascii
        	/* save translation */
			KASSERT(uc < ASCII_TABLE_SIZE)
        	font.ascii_table[uc] = glyph;
		}
        s++;
    }

	calc_glyphs_caches(&font);
	return font;
}

bool write_PSF_char(const PSFFont font, const unsigned char c, const Vector2i position, Pixel buffer[], const Vector2i buffer_size,
		const Color background_color, const Color font_color){
	//if it's an extended ascii 
	if(c & 128)
		return false;
	uint16_t unicode = c;
	if(font.has_table){
		KASSERT(c < ASCII_TABLE_SIZE)
		unicode = font.ascii_table[c];
	}
	KASSERT(font.glyphs != NULL);
	CachedGlyph cached_glyph = font.glyphs[unicode];
	uint64_t display_offset = position.y * buffer_size.x + position.x;

	Pixel font_pixel = color_to_pixel(font_color);
	Pixel background_pixel = color_to_pixel(background_color);

	Pixel fast_pixel_arr[2] = {background_pixel, font_pixel};

	uint64_t font_width = font.header->width;
	uint64_t font_height = font.header->height;
	for(uint32_t y = 0; y < font_height; y++){
		for(uint32_t x = 0; x < font_width; x++){
			KASSERT(cached_glyph.pixels != NULL);
			buffer[display_offset+x] = fast_pixel_arr[cached_glyph.pixels[y*font_width+x]];
		}

		//next line to display
		display_offset += buffer_size.x;
	}
	return true;
}
