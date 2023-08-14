#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../kdefs.h"


void memset(volatile void* dst, uint8_t data, size_t size);
void memcpy(void* dst, const void* src, size_t size);
