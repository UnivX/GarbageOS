#include "kio.h"
#include "mem/heap.h"

static KioState kio_state;
static bool is_initialized = false;

static void unsync_putchar(char c, bool flush);
static void unsync_opt_flush();
static void unsync_flush();

void init_kio(const DisplayInterface display, const PSFFont font, const Color background_color, const Color font_color){
	init_spinlock(&kio_state.lock);
	is_initialized = true;
	kio_state.buffer = kmalloc(display.info.height*display.info.width*sizeof(Pixel));
	kio_state.display = display;
	kio_state.font_color = font_color;
	kio_state.background_color = background_color;
	kio_state.next_x = 0;
	kio_state.next_y = 0;
	kio_state.font = font;
	kio_state.screen_size = display.info.height*display.info.width;
	kio_state.char_buffer = make_circular_buffer(KIO_CIRCULAR_BUFFER_SIZE);

	Pixel background_pixel = color_to_pixel(background_color);
	for(unsigned int i = 0; i < kio_state.screen_size; i++){
		kio_state.buffer[i] = background_pixel;
	}
}

bool is_kio_initialized(){
	return is_initialized;
}

void print(const char* str){
	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);
	while(*str != 0){
		unsync_putchar(*str, false);
		++str;
	}
	unsync_opt_flush();
	RELEASE_SPINLOCK_HARD(&kio_state.lock);
	return;
}

void unsync_print(const char* str){
	while(*str != 0){
		unsync_putchar(*str, false);
		++str;
	}
	return;
}

void unsync_write_char(char c){
	if(kio_state.next_x + kio_state.font.header->width >= kio_state.display.info.width || c == '\n'){
		//next line
		if(kio_state.next_y+kio_state.font.header->height*2 >= kio_state.display.info.height){
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

	if(c != '\n'){
		Vector2i pos = {kio_state.next_x, kio_state.next_y};
		Vector2i buff_size = {kio_state.display.info.width, kio_state.display.info.height};
		write_PSF_char(kio_state.font, c, pos, kio_state.buffer, buff_size, kio_state.background_color, kio_state.font_color);
		kio_state.next_x += kio_state.font.header->width;
	}
}

void unsync_flush(){
	if(is_circular_buffer_empty(&kio_state.char_buffer))
		return;

	while(!is_circular_buffer_empty(&kio_state.char_buffer)){
		unsync_write_char(read_circular_buffer(&kio_state.char_buffer));
	}
	kio_state.display.write_pixels(kio_state.display, kio_state.buffer, kio_state.screen_size);
}

void unsync_opt_flush(){
	const uint64_t max_chars = 32;
	if(circular_buffer_written_bytes(&kio_state.char_buffer) >= max_chars){
		unsync_flush();
	}
}

void kio_flush(){
	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);
	unsync_flush();
	RELEASE_SPINLOCK_HARD(&kio_state.lock);
}

void unsync_putchar(char c, bool flush){
	write_circular_buffer(&kio_state.char_buffer, c);

	if(flush){
		unsync_flush();
	}
	return;
}

void putchar(char c, bool flush){
	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);
	unsync_putchar(c, flush);
	RELEASE_SPINLOCK_HARD(&kio_state.lock);
}

void finalize_kio(){
	if(!is_initialized)
		kpanic(KIO_ERROR);
	kio_flush();
	kio_state.display.finalize(kio_state.display);
	kfree(kio_state.buffer);
	destory_circular_buffer(&kio_state.char_buffer);
}

uint64_t fill_digit_buffer(uint64_t n, uint64_t base, uint8_t* buffer, uint64_t buffer_size){
	for(unsigned int i = 0; i < buffer_size; i++)
		buffer[i]=0;

	uint8_t counter = 0;
	while(n != 0){
		uint8_t digit = n % base;
		n /= base;
		KASSERT(counter < buffer_size);
		buffer[counter] = digit;
		counter++;
	}
	return counter;
}

void print_uint64_hex(uint64_t n){

	uint8_t buffer[sizeof(n)*2];
	fill_digit_buffer(n, 16, buffer, sizeof(n)*2);

	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);
	unsync_putchar('0', false);
	unsync_putchar('x', false);
	for(int i = sizeof(n)*2-1; i >= 0; i--){
		int digit = buffer[i];
		if(digit >= 10){
			unsync_putchar('A'+digit-10, false);
		}else{
			unsync_putchar('0'+digit, false);
		}
	}
	unsync_opt_flush();
	RELEASE_SPINLOCK_HARD(&kio_state.lock);
}


void print_uint64_dec(uint64_t n){
	const uint64_t buffer_size = 128;
	uint8_t buffer[buffer_size];
	uint64_t written_digits_count = fill_digit_buffer(n, 10, buffer, buffer_size);

	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);
	if(n == 0)
		unsync_putchar('0', false);

	for(int i = written_digits_count-1; i >= 0; i--){
		int digit = buffer[i];
		unsync_putchar('0'+digit, false);
	}

	unsync_opt_flush();
	RELEASE_SPINLOCK_HARD(&kio_state.lock);
}

void unsync_print_uint64_hex(uint64_t n){
	uint8_t buffer[sizeof(n)*2];
	fill_digit_buffer(n, 16, buffer, sizeof(n)*2);

	unsync_putchar('0', false);
	unsync_putchar('x', false);
	for(int i = sizeof(n)*2-1; i >= 0; i--){
		int digit = buffer[i];
		if(digit >= 10){
			unsync_putchar('A'+digit-10, false);
		}else{
			unsync_putchar('0'+digit, false);
		}
	}
}


void unsync_print_uint64_dec(uint64_t n){
	const uint64_t buffer_size = 128;
	uint8_t buffer[buffer_size];
	uint64_t written_digits_count = fill_digit_buffer(n, 10, buffer, buffer_size);

	if(n == 0)
		unsync_putchar('0', false);

	for(int i = written_digits_count-1; i >= 0; i--){
		int digit = buffer[i];
		unsync_putchar('0'+digit, false);
	}
}

static bool substr_cmp(const char* string, const char* substr){
	while(*substr != 0){

		if(*string == 0)
			return false;

		if(*string != *substr)
			return false;

		string++;
		substr++;
	}
	return true;
}

void printf(const char* format, ...){
	bool token_found = false;

	va_list ptr;
	va_start(ptr, format);

	ACQUIRE_SPINLOCK_HARD(&kio_state.lock);

	while(*format != 0){
		if(token_found){
			if(*format == '%'){
				unsync_putchar('%', false);
			}else{
				//if we are parsing a format token
				if(substr_cmp(format, "u64")){
					format+=2;
					unsync_print_uint64_dec(va_arg(ptr, uint64_t));
				}else if(substr_cmp(format, "h64")){
					format+=2;
					unsync_print_uint64_hex(va_arg(ptr, uint64_t));
				}else if(substr_cmp(format, "s")){
					unsync_print(va_arg(ptr, const char*));
				}
			}
			token_found = false;
		}else{
			//if we are elaborating text
			//check if it's a format token start
			if(*format == '%'){
				token_found = true;
			}else{
				//write char
				unsync_putchar(*format, false);
			}
		}
		format++;
	}
	unsync_opt_flush();
	RELEASE_SPINLOCK_HARD(&kio_state.lock);

	va_end(ptr);
	return;
}
