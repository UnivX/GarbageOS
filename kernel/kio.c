#include "kio.h"

KioState kio_state;
static bool is_initialized = false;

void init_kio(DisplayInterface display, PSFFont font, Pixel background_color, Pixel font_color){
	KASSERT(display.info.height*display.info.width <= BUFFER_SIZE)
	is_initialized = true;
	kio_state.display = display;
	kio_state.font_color = font_color;
	kio_state.background_color = background_color;
	kio_state.next_x = 0;
	kio_state.next_y = 0;
	kio_state.font = font;

	for(int i = 0; i < BUFFER_SIZE; i++){
		kio_state.buffer[i] = background_color;
	}
}

void print(char* str){
	while(*str != 0){
		putchar(*str, false);
		++str;
	}
	kio_state.display.write_pixels(kio_state.buffer, BUFFER_SIZE);
	return;
}

void putchar(char c, bool flush){
	if(kio_state.next_x + kio_state.font.header->width >= kio_state.display.info.width || c == '\n'){
		//next line
		kio_state.next_y += kio_state.font.header->height;
		kio_state.next_x = 0;
	}
	Vector2i pos = {kio_state.next_x, kio_state.next_y};
	Vector2i buff_size = {kio_state.display.info.width, kio_state.display.info.height};
	if(c != '\n'){
		write_PSF_char(kio_state.font, c, pos, kio_state.buffer, buff_size, kio_state.background_color, kio_state.font_color);
		kio_state.next_x += kio_state.font.header->width;
	}
	if(flush)
		kio_state.display.write_pixels(kio_state.buffer, BUFFER_SIZE);
	return;
}

void finalize_kio(){
	if(!is_initialized)
		kpanic(KIO_ERROR);
	kio_state.display.finalize();
}
