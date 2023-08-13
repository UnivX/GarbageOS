#include "heap.h"

uint64_t get_fixed_heap_chunk_size(HeapChunkHeader* header){
	return header->size & (~7);
}

bool get_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	return (header->size & (uint64_t)flag) != 0;
}

void set_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	header->size &= flag;
}

HeapChunkHeader* get_next_heap_chunk_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += get_fixed_heap_chunk_size(header);
	raw_header += sizeof(HeapChunkHeader) + sizeof(HeapChunkFooter);
	return (HeapChunkHeader*)raw_header;
}

HeapChunkHeader* get_prev_heap_chunk_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header -= sizeof(HeapChunkFooter);
	HeapChunkFooter* prev_footer = (HeapChunkFooter*)raw_header;
	return get_heap_chunk_header_from_footer(prev_footer);
}

void* get_heap_chunk_user_data(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += sizeof(HeapChunkHeader);
	return raw_header;
}

void set_bucket_list_next(HeapChunkHeader* header, HeapChunkHeader* next){
	//assert that the chunk is free and that we are not going to override some user data
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
	HeapChunkHeader** ptr_to_next = (HeapChunkHeader**)get_heap_chunk_user_data(header);
	*ptr_to_next = next;
}

HeapChunkHeader* get_bucket_list_next(HeapChunkHeader* header){
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
	HeapChunkHeader** ptr_to_next = (HeapChunkHeader**)get_heap_chunk_user_data(header);
	return *ptr_to_next;
}

bool is_heap_chunk_corrupted(HeapChunkHeader* header){
	//check if the two sizes are different
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(header);
	return get_fixed_heap_chunk_size_from_footer(footer) != get_fixed_heap_chunk_size(header);
}

HeapChunkHeader* get_heap_chunk_header_from_footer(HeapChunkFooter* footer){
	void* raw_footer = (void*)footer;
	raw_footer -= get_fixed_heap_chunk_size_from_footer(footer);
	raw_footer -= sizeof(HeapChunkHeader);
	return (HeapChunkHeader*)raw_footer;
}

HeapChunkFooter* get_heap_chunk_footer_from_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += sizeof(HeapChunkHeader);
	raw_header += get_fixed_heap_chunk_size(header);
	HeapChunkFooter* footer = (HeapChunkFooter*)raw_header;
	return footer;
}

uint64_t get_fixed_heap_chunk_size_from_footer(HeapChunkFooter* footer){
	return footer->size & (~7);
}

void initizialize_heap(Heap* heap, void* start, uint64_t size){
	//assert the memory is aligned
	KASSERT(size > 128);
	KASSERT(((uint64_t)start) % 8 == 0);
	KASSERT(size % 8 == 0);

	//initialize data
	for(int i = 0; i < BUCKETS_COUNT; i++)
		heap->free_buckets[i] = NULL;
	heap->enable_growth = false;
	heap->start = start;
	heap->size = size;

	//create a big chunk
	uint64_t user_data_size = size - sizeof(HeapChunkFooter) - sizeof(HeapChunkHeader);

	HeapChunkHeader* chunk_header = (HeapChunkHeader*)start;
	chunk_header->size = user_data_size;
	set_heap_chunk_flag(chunk_header, HEAP_WILDERNESS_CHUNK);
	set_heap_chunk_flag(chunk_header, HEAP_FREE_CHUNK);
	set_bucket_list_next(chunk_header, NULL);

	HeapChunkFooter* chunk_footer = get_heap_chunk_footer_from_header(chunk_header);
	chunk_footer->size = user_data_size;

	heap->wilderness_chunk = chunk_header;
	heap->free_buckets[BUCKETS_COUNT-1] = chunk_header;
}

bool is_first_chunk_of_heap(Heap* heap, HeapChunkHeader* header){
	return heap->start == (void*)header;
}
bool is_last_chunk_of_heap(HeapChunkHeader* header){
	return get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
}
void enable_heap_growth(Heap* heap){
	heap->enable_growth = true;
}
