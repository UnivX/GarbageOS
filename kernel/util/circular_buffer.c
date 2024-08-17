#include "circular_buffer.h"

CircularBuffer make_circular_buffer(uint64_t size){
	CircularBuffer buff;
	buff.buffer = kmalloc(size);
	buff.buffer_size = size;
	buff.read_idx = 0;
	buff.write_idx = 0;
	buff.data_lost = false;
	return buff;
}

void destory_circular_buffer(CircularBuffer* buff){
	kfree(buff->buffer);
	buff->buffer = NULL;
}

bool is_circular_buffer_empty(CircularBuffer* buff){
	return buff->read_idx == buff->write_idx;
}

void write_circular_buffer(CircularBuffer* buff, uint8_t b){
	buff->data_lost |= buff->old_write_idx-1 == buff->read_idx && buff->write_idx == buff->read_idx;
	buff->buffer[buff->write_idx] = b;
	buff->old_write_idx = buff->write_idx;
	buff->write_idx = (buff->write_idx+1) % buff->buffer_size;
}

uint8_t read_circular_buffer(CircularBuffer* buff){
	KASSERT(!is_circular_buffer_empty(buff));
	uint8_t b = buff->buffer[buff->read_idx];
	buff->read_idx = (buff->read_idx+1) % buff->buffer_size;
	return b;
}

bool has_circular_buffer_lost_data(CircularBuffer* buff){
	bool data_lost = buff->data_lost;
	buff->data_lost = false;
	return data_lost;
}

uint64_t circular_buffer_written_bytes(CircularBuffer* buff){
	if(buff->write_idx > buff->read_idx)
		return buff->write_idx - buff->read_idx;
	return buff->buffer_size - buff->read_idx + buff->write_idx;
}
