#include "../../hal.h"
#include <stdbool.h>

inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ( "inl %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

inline void outb(uint16_t port, uint8_t data){
	asm volatile ( "outb %0, %1" : : "a"(data), "Nd"(port) :"memory");
}

inline void outw(uint16_t port, uint16_t data){
	asm volatile ( "outw %0, %1" : : "a"(data), "Nd"(port) :"memory");
}

inline void outl(uint16_t port, uint32_t data){
	asm volatile ( "outl %0, %1" : : "a"(data), "Nd"(port) :"memory");
}

inline void io_wait(){
	outb(0x80, 0);
}

inline void enable_interrupts(){
	asm volatile("sti");
}

inline void disable_interrupts(){
	asm volatile("cli");
}

inline void halt(){
	disable_interrupts();
	volatile bool wtrue = true;
	while(wtrue){
		asm volatile("hlt" : : : "memory");
		wtrue = true;
	}
	return;
}

void memset(volatile void* dst, uint8_t data, size_t size){
	for(size_t i = 0; i < size; i++)
		((volatile uint8_t*)dst)[i] = data;
}

void memcpy(void* dst, const void* src, size_t size){
	uint64_t* uisrc = (uint64_t*)(src);
	uint64_t* uidst = (uint64_t*)(dst);
	uint64_t byte_index = 0;
	uint64_t aligned_size = size - (size % 64);
	while(byte_index < aligned_size){
		uidst[0+(byte_index/8)] = uisrc[0+(byte_index/8)];
		uidst[1+(byte_index/8)] = uisrc[1+(byte_index/8)];
		uidst[2+(byte_index/8)] = uisrc[2+(byte_index/8)];
		uidst[3+(byte_index/8)] = uisrc[3+(byte_index/8)];
		uidst[4+(byte_index/8)] = uisrc[4+(byte_index/8)];
		uidst[5+(byte_index/8)] = uisrc[5+(byte_index/8)];
		uidst[6+(byte_index/8)] = uisrc[6+(byte_index/8)];
		uidst[7+(byte_index/8)] = uisrc[7+(byte_index/8)];
		byte_index+=64;
	}

	while(byte_index < size){
		((uint8_t*)uidst)[byte_index] = ((uint8_t*)uisrc)[byte_index];
		byte_index++;
	}

	/*
	asm volatile ("rep movsb"
                : "=D" (dst),
                  "=S" (src),
                  "=c" (size)
                : "0" (dst),
                  "1" (src),
                  "2" (size)
                : "memory");
	*/
}
