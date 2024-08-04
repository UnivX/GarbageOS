#include "psf.h"

PSFFont get_default_PSF_font(){
	PSFFont invalid_font = {NULL, {}, false, false};

	PSFFont font;
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
	return font;
}

bool write_PSF_char(const PSFFont font, const unsigned char c, const Vector2i position, Pixel buffer[], const Vector2i buffer_size,
		const Color background_color, const Color font_color){
	//if it's an extended ascii 
	if(c & 128)
		return false;
	int line_in_bytes = (font.header->width+7)/8;//divide by 8 and add 1 if there is a remainder
	uint16_t unicode = c;
	if(font.has_table){
		KASSERT(c < ASCII_TABLE_SIZE)
		unicode = font.ascii_table[c];
	}
	uint8_t *glyph = (uint8_t*)font.header+ font.header->headersize + unicode*font.header->bytesperglyph;
	uint64_t display_offset = position.y * buffer_size.x + position.x;

	Pixel font_pixel = color_to_pixel(font_color);
	Pixel background_pixel = color_to_pixel(background_color);

	Pixel fast_pixel_arr[2] = {background_pixel, font_pixel};

	for(uint32_t y = 0; y < font.header->height; y++){
		for(uint32_t x = 0; x < font.header->width; x++){
			KASSERT((int64_t)(display_offset+x) < buffer_size.x*buffer_size.y);
			bool is_font_pixel = (glyph[x/8] << (x%8))&0x80;
			buffer[display_offset+x] = fast_pixel_arr[is_font_pixel];
			/*
			if(is_font_pixel)
				buffer[display_offset+x] = font_pixel;
			else
				buffer[display_offset+x] = background_pixel;
			*/
		}

		//next line to display
		display_offset += buffer_size.x;
		glyph += line_in_bytes;
	}
	
	return true;
}
