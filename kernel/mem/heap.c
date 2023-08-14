#include "heap.h"

uint64_t get_fixed_heap_chunk_size(HeapChunkHeader* header){
	return header->size & (~7);
}

bool get_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	return (header->size & (uint64_t)flag) != 0;
}

void set_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	header->size |= flag;
}

void reset_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	header->size &= ~flag;
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

static void set_bucket_list_prev(HeapChunkHeader* header, HeapChunkHeader* prev){
	//assert that the chunk is free and that we are not going to override some user data
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
	HeapChunkHeader** ptr_to_prev = (HeapChunkHeader**)(get_heap_chunk_user_data(header)+8);
	*ptr_to_prev = prev;
}

static HeapChunkHeader* get_bucket_list_prev(HeapChunkHeader* header){
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
	HeapChunkHeader** prev_to_next = (HeapChunkHeader**)(get_heap_chunk_user_data(header)+8);
	return *prev_to_next;
}

bool is_heap_chunk_corrupted(HeapChunkHeader* header){
	//check if the two sizes are different
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(header);
	if(get_fixed_heap_chunk_size_from_footer(footer) != get_fixed_heap_chunk_size(header))
		return false;
	if((footer->size & 7) != 0)
		return false;
	return true;
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
	HeapChunkFooter* chunk_footer = get_heap_chunk_footer_from_header(chunk_header);
	chunk_footer->size = user_data_size;
	heap->wilderness_chunk = chunk_header;
	add_to_bucket_list(heap, chunk_header);
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

void merge_with_next_chunk(Heap* heap, HeapChunkHeader* header){
	KASSERT(!is_last_chunk_of_heap(header));
	HeapChunkHeader* next = get_next_heap_chunk_header(header);
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));
	KASSERT(get_heap_chunk_flag(next, HEAP_FREE_CHUNK));
	
	//remove the two from the bucket list
	remove_from_bucket_list(heap, header);
	remove_from_bucket_list(heap, next);

	//the next header and the old footer will be overrided so the new size will have them
	uint64_t next_size = get_fixed_heap_chunk_size(next);
	uint64_t header_size = get_fixed_heap_chunk_size(header);
	uint64_t new_size = next_size + header_size + sizeof(HeapChunkFooter) + sizeof(HeapChunkHeader);
	KASSERT(new_size % 8 == 0);
	
	//set the new header
	header->size = new_size;
	set_heap_chunk_flag(header, HEAP_FREE_CHUNK);
	//if the next is the wilderness chunk then set the merged one to be the new
	if(get_heap_chunk_flag(next, HEAP_WILDERNESS_CHUNK)){
		set_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
		heap->wilderness_chunk = header;
	}

	//set the new footer
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(next);
	footer->size = new_size;

	//add the new chunk to the bucket list
	add_to_bucket_list(heap, header);
}

 bool split_chunk_and_alloc(Heap* heap, HeapChunkHeader* header, uint64_t first_chunk_size){
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));
	KASSERT(first_chunk_size % 8 == 0)

	int64_t sencond_chunk_size = get_fixed_heap_chunk_size(header);
	sencond_chunk_size -= first_chunk_size + sizeof(HeapChunkFooter) + sizeof(HeapChunkHeader);
	if(sencond_chunk_size < HEAP_CHUNK_MIN_SIZE)
		return false;
	KASSERT(sencond_chunk_size % 8 == 0)

	bool is_header_wilderness_chunk = get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
	remove_from_bucket_list(heap, header);

	//create the first chunk
	header->size = first_chunk_size;//do not set the free chunk flag
	HeapChunkFooter* first_chunk_footer = get_heap_chunk_footer_from_header(header);
	first_chunk_footer->size = first_chunk_size;
	//do not add to the bucket because it will not be free

	//create the second_chunk
	//the next chunk ptr is caluculated from the data from the header, so it's possibile to just write this
	HeapChunkHeader* second_chunk = get_next_heap_chunk_header(header);
	second_chunk->size = (uint64_t)sencond_chunk_size;
	set_heap_chunk_flag(second_chunk, HEAP_FREE_CHUNK);
	if(is_header_wilderness_chunk){
		set_heap_chunk_flag(second_chunk, HEAP_WILDERNESS_CHUNK);
		heap->wilderness_chunk = second_chunk;
	}
	HeapChunkFooter* sencond_chunk_footer = get_heap_chunk_footer_from_header(second_chunk);
	sencond_chunk_footer->size = (uint64_t)sencond_chunk_size;
	add_to_bucket_list(heap, header);

	return true;
}

void remove_from_bucket_list(Heap* heap, HeapChunkHeader* header){
	int index = get_bucket_index_from_header(header);
	HeapChunkHeader* prev_list_elem = get_bucket_list_prev(header);
	HeapChunkHeader* next_list_elem = get_bucket_list_next(header);

	//if it's the first in the list remove it from it
	if(heap->free_buckets[index] == header)
		heap->free_buckets[index] = next_list_elem;//even if NULL it's good
	

	//TODO: possible optimization
	//link the prev with the next 
	if(prev_list_elem != NULL && next_list_elem != NULL){
		set_bucket_list_next(prev_list_elem, next_list_elem);
		set_bucket_list_prev(next_list_elem, prev_list_elem);
	}else{
		if(prev_list_elem == NULL && next_list_elem != NULL){
			set_bucket_list_prev(next_list_elem, NULL);
		}else if(prev_list_elem != NULL && next_list_elem == NULL){
			set_bucket_list_next(prev_list_elem, NULL);
		}
	}

	//remove the links from this one
	set_bucket_list_next(header, NULL);
	set_bucket_list_prev(header, NULL);
}

void add_to_bucket_list(Heap* heap, HeapChunkHeader* header){
	int index = get_bucket_index_from_header(header);
	set_bucket_list_prev(header, NULL);
	set_bucket_list_next(header, heap->free_buckets[index]);
	if(heap->free_buckets[index] != NULL)
		set_bucket_list_prev(heap->free_buckets[index], header);
	heap->free_buckets[index] = header;
}

//TODO: do a better classification
int get_bucket_index_from_size(uint64_t size){
	if(size == 16)
		return 0;
	if(size == 24)
		return 1;
	if(size == 32)
		return 2;

	if(size <= (1<<21)-1){
		//logarithmic growth till 2^14
		int idx = -5+3;//32 = 2^5
		uint64_t cpy = size;
		while(cpy != 1){
			cpy >>= 1;
			idx++;
		}
		KASSERT(idx > 2);
		KASSERT(idx <= 20);
		return idx;
	}
	const uint64_t step = 512*MB / (BUCKETS_COUNT-14);
	uint64_t index = (size / step) + 14;
	if(index > BUCKETS_COUNT-1)
		index = BUCKETS_COUNT-1;
	return index;
}

int get_bucket_index_from_header(HeapChunkHeader* header){
	//if it's the wild chunk return the last
	if(get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK))
		return BUCKETS_COUNT-1;
	return get_bucket_index_from_size(get_fixed_heap_chunk_size(header));
}

void alloc_chunk(Heap* heap, HeapChunkHeader* header){
	KASSERT(!get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK));
	remove_from_bucket_list(heap, header);
	reset_heap_chunk_flag(header, HEAP_FREE_CHUNK);
}
