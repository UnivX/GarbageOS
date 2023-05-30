#pragma once
#include <stdint.h>

#define KB 1024
#define MB 1024*1024
#define GB MB*KB
#define TB GB*KB

#define FRAME_ALLOCATOR_ERROR 1
#define VBE_ERROR 2
void kpanic(volatile uint64_t error_code);
