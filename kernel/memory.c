#include "memory.h"

void memset(volatile void* dst, uint8_t data, size_t size){
	for(size_t i = 0; i < size; i++)
		((volatile uint8_t*)dst)[i] = data;
}
