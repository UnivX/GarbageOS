#pragma once
#include "../mem/heap.h"
#include "../kdefs.h"
#include "../hal.h"

typedef struct CircularBuffer{
	uint8_t* buffer;
	uint64_t buffer_size;

	uint64_t read_idx;
	uint64_t old_write_idx;
	uint64_t write_idx;

	bool data_lost;
} CircularBuffer;

CircularBuffer make_circular_buffer(uint64_t size);
void destory_circular_buffer(CircularBuffer* buff);
bool is_circular_buffer_empty(CircularBuffer* buff);
void write_circular_buffer(CircularBuffer* buff, uint8_t b);
uint8_t read_circular_buffer(CircularBuffer* buff);
bool has_circular_buffer_lost_data(CircularBuffer* buff);
uint64_t circular_buffer_written_bytes(CircularBuffer* buff);
