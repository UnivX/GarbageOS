#include "kio.h"

KioState kio_state;
static bool is_initialized = false;

void init_kio(DisplayInterface display, PSFFont font, Color background_color, Color font_color){
	KASSERT(display.info.height*display.info.width <= BUFFER_SIZE) is_initialized = true;
	kio_state.display = display;
	kio_state.font_color = font_color;
	kio_state.background_color = background_color;
	kio_state.next_x = 0;
	kio_state.next_y = 0;
	kio_state.font = font;
	kio_state.screen_size = display.info.height*display.info.width;

	Pixel background_pixel = color_to_pixel(background_color);
	for(unsigned int i = 0; i < kio_state.screen_size; i++){
		kio_state.buffer[i] = background_pixel;
	}
}

void print(char* str){
	while(*str != 0){
		putchar(*str, false);
		++str;
	}
	kio_state.display.write_pixels(kio_state.buffer, kio_state.screen_size);
	return;
}

void putchar(char c, bool flush){
	if(kio_state.next_x + kio_state.font.header->width >= kio_state.display.info.width || c == '\n'){
		//next line
		if(kio_state.next_y+kio_state.font.header->height >= kio_state.display.info.height){
			kio_state.next_y -= kio_state.font.header->height;
			uint64_t offset = kio_state.display.info.width*kio_state.font.header->height;
			for(unsigned int i = offset; i < kio_state.screen_size; i++)
				kio_state.buffer[i-offset] = kio_state.buffer[i];

			for(unsigned int i = kio_state.screen_size-offset; i < kio_state.screen_size; i++)
				kio_state.buffer[i] = color_to_pixel(kio_state.background_color);
		}
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
		kio_state.display.write_pixels(kio_state.buffer, kio_state.screen_size);
	return;
}

void finalize_kio(){
	if(!is_initialized)
		kpanic(KIO_ERROR);
	kio_state.display.finalize();
}

void print_uint64_hex(uint64_t n){
	putchar('0', false);
	putchar('x', false);

	uint8_t buffer[sizeof(n)*2];
	for(unsigned int i = 0; i < sizeof(n)*2; i++)
		buffer[i]=0;

	uint8_t counter = 0;
	while(n != 0){
		int digit = n % 16;
		n /= 16;
		buffer[counter] = digit;
		counter++;
	}

	for(int i = sizeof(n)*2-1; i >= 0; i--){
		int digit = buffer[i];
		if(digit >= 10){
			putchar('A'+digit-10, false);
		}else{
			putchar('0'+digit, false);
		}
	}
	kio_state.display.write_pixels(kio_state.buffer, kio_state.screen_size);
}
void print_uint64_dec(uint64_t n){
	const uint64_t buffer_size = 128;
	uint8_t buffer[buffer_size];

	if(n == 0)
		putchar('0', false);

	for(unsigned int i = 0; i < sizeof(n)*2; i++)
		buffer[i]=0;

	uint8_t counter = 0;
	while(n != 0){
		int digit = n % 10;
		n /= 10;
		KASSERT(counter < buffer_size);
		buffer[counter] = digit;
		counter++;
	}

	for(int i = counter-1; i >= 0; i--){
		int digit = buffer[i];
		putchar('0'+digit, false);
	}
	kio_state.display.write_pixels(kio_state.buffer, kio_state.screen_size);
}
