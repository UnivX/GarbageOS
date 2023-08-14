//early allocation for data like system tables(e.g. IDT, GDT, ...)
#include "../hal.h"
#include "../kdefs.h"
#define EARLY_ALLOCATOR_SIZE 16*KB
void* early_alloc(size_t size);
