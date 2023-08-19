#include "heap.h"

static uint64_t get_fixed_heap_chunk_size(HeapChunkHeader* header);
static bool get_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag);
static void set_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag);
static void reset_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag);
static HeapChunkHeader* get_next_heap_chunk_header(HeapChunkHeader* header);
static HeapChunkHeader* get_prev_heap_chunk_header(HeapChunkHeader* header);
static void* get_heap_chunk_user_data(HeapChunkHeader* header);
static void set_bucket_list_next(HeapChunkHeader* header, HeapChunkHeader* next);
static HeapChunkHeader* get_bucket_list_next(HeapChunkHeader* header);
static void set_bucket_list_prev(HeapChunkHeader* header, HeapChunkHeader* prev);
static HeapChunkHeader* get_bucket_list_prev(HeapChunkHeader* header);
static bool is_heap_chunk_corrupted(HeapChunkHeader* header);
static HeapChunkHeader* get_heap_chunk_from_user_data(void* user_data);

static uint64_t get_fixed_heap_chunk_size_from_footer(HeapChunkFooter* footer);
static HeapChunkHeader* get_heap_chunk_header_from_footer(HeapChunkFooter* footer);
static HeapChunkFooter* get_heap_chunk_footer_from_header(HeapChunkHeader* header);

static void initizialize_heap(Heap* heap, void* start, uint64_t size);
static void enable_heap_growth(Heap* heap);
static bool is_first_chunk_of_heap(Heap* heap, HeapChunkHeader* header);
static bool is_last_chunk_of_heap(HeapChunkHeader* header);
static void merge_with_next_chunk(Heap* heap, HeapChunkHeader* header);
static void merge_with_next_and_prev_chunk(Heap* heap, HeapChunkHeader* header);

static bool split_chunk_and_alloc(Heap* heap, HeapChunkHeader* header, uint64_t first_chunk_size);
static void remove_from_bucket_list(Heap* heap, HeapChunkHeader* header);
static void add_to_bucket_list(Heap* heap, HeapChunkHeader* header);
static int get_bucket_index_from_size(uint64_t size);
static int get_bucket_index_from_header(HeapChunkHeader* header);
static void alloc_chunk(Heap* heap, HeapChunkHeader* header);

static uint64_t get_fixed_heap_chunk_size(HeapChunkHeader* header){
	return header->size & (~7);
}

static bool get_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	return (header->size & (uint64_t)flag) != 0;
}

static void set_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	header->size |= flag;
}

static void reset_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag){
	header->size &= ~flag;
}

static HeapChunkHeader* get_next_heap_chunk_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += get_fixed_heap_chunk_size(header);
	raw_header += sizeof(HeapChunkHeader) + sizeof(HeapChunkFooter);
	return (HeapChunkHeader*)raw_header;
}

static HeapChunkHeader* get_prev_heap_chunk_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header -= sizeof(HeapChunkFooter);
	HeapChunkFooter* prev_footer = (HeapChunkFooter*)raw_header;
	return get_heap_chunk_header_from_footer(prev_footer);
}

static void* get_heap_chunk_user_data(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += sizeof(HeapChunkHeader);
	return raw_header;
}

static void set_bucket_list_next(HeapChunkHeader* header, HeapChunkHeader* next){
	//assert that the chunk is free and that we are not going to override some user data
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
	HeapChunkHeader** ptr_to_next = (HeapChunkHeader**)get_heap_chunk_user_data(header);
	*ptr_to_next = next;
}

static HeapChunkHeader* get_bucket_list_next(HeapChunkHeader* header){
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

static bool is_heap_chunk_corrupted(HeapChunkHeader* header){
	//check if the two sizes are different
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(header);
	uint64_t size1 = get_fixed_heap_chunk_size_from_footer(footer);
	uint64_t size2 = get_fixed_heap_chunk_size(header);
	if(size1 != size2)
		return true;
	if((footer->size & 7) != 0)
		return true;
	return false;
}

static HeapChunkHeader* get_heap_chunk_header_from_footer(HeapChunkFooter* footer){
	void* raw_footer = (void*)footer;
	raw_footer -= get_fixed_heap_chunk_size_from_footer(footer);
	raw_footer -= sizeof(HeapChunkHeader);
	return (HeapChunkHeader*)raw_footer;
}

static HeapChunkFooter* get_heap_chunk_footer_from_header(HeapChunkHeader* header){
	void* raw_header = (void*)header;
	raw_header += sizeof(HeapChunkHeader);
	raw_header += get_fixed_heap_chunk_size(header);
	HeapChunkFooter* footer = (HeapChunkFooter*)raw_header;
	return footer;
}

static HeapChunkHeader* get_heap_chunk_from_user_data(void* user_data){
	user_data -= sizeof(HeapChunkHeader);
	return (HeapChunkHeader*)(user_data);
}

static uint64_t get_fixed_heap_chunk_size_from_footer(HeapChunkFooter* footer){
	return footer->size & (~7);
}

static void initizialize_heap(Heap* heap, void* start, uint64_t size){
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
	heap->number_of_chunks = 1;

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

static bool is_first_chunk_of_heap(Heap* heap, HeapChunkHeader* header){
	return heap->start == (void*)header;
}
static bool is_last_chunk_of_heap(HeapChunkHeader* header){
	return get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
}
static void enable_heap_growth(Heap* heap){
	heap->enable_growth = true;
}

static void merge_with_next_chunk(Heap* heap, HeapChunkHeader* header){
	KASSERT(!is_last_chunk_of_heap(header));
	HeapChunkHeader* next = get_next_heap_chunk_header(header);
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));
	KASSERT(get_heap_chunk_flag(next, HEAP_FREE_CHUNK));
	bool is_wilderness_chunk = get_heap_chunk_flag(next, HEAP_WILDERNESS_CHUNK);

	//keep the adress to check if it's the same after
#ifdef HEAP_DEBUG
	HeapChunkFooter* old_last_footer = get_heap_chunk_footer_from_header(next);
#endif
	
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
	if(is_wilderness_chunk){
		set_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
		heap->wilderness_chunk = header;
	}

	//set the new footer
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(next);
	footer->size = new_size;

	//check if they are the same
#ifdef HEAP_DEBUG
	KASSERT(footer == old_last_footer);
#endif

	//add the new chunk to the bucket list
	add_to_bucket_list(heap, header);
	heap->number_of_chunks--;
}
static void merge_with_next_and_prev_chunk(Heap* heap, HeapChunkHeader* header){
	KASSERT(!is_last_chunk_of_heap(header));
	KASSERT(!is_first_chunk_of_heap(heap, header));
	HeapChunkHeader* next = get_next_heap_chunk_header(header);
	HeapChunkHeader* prev = get_prev_heap_chunk_header(header);

	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));
	KASSERT(get_heap_chunk_flag(prev, HEAP_FREE_CHUNK));
	KASSERT(get_heap_chunk_flag(next, HEAP_FREE_CHUNK));

	//keep the adress to check if it's the same after
#ifdef HEAP_DEBUG
	HeapChunkFooter* old_last_footer = get_heap_chunk_footer_from_header(next);
#endif

	bool is_wilderness_chunk = get_heap_chunk_flag(next, HEAP_WILDERNESS_CHUNK);

	//remove all tree from the bucket lists
	remove_from_bucket_list(heap, header);
	remove_from_bucket_list(heap, next);
	remove_from_bucket_list(heap, prev);

	//calculate the new size
	//add to the new size the size of the overrided footer and header
	uint64_t new_size = sizeof(HeapChunkFooter)*2 + sizeof(HeapChunkHeader)*2;
	new_size += get_fixed_heap_chunk_size(header);
	new_size += get_fixed_heap_chunk_size(next);
	new_size += get_fixed_heap_chunk_size(prev);
	KASSERT(new_size % 8 == 0);

	//set the new_header
	prev->size = new_size;
	set_heap_chunk_flag(prev, HEAP_FREE_CHUNK);
	if(is_wilderness_chunk){
		set_heap_chunk_flag(prev, HEAP_WILDERNESS_CHUNK);
		heap->wilderness_chunk = prev;
	}
	
	//set the new footer
	HeapChunkFooter* footer = get_heap_chunk_footer_from_header(prev);
	footer->size = new_size;

	//check if they are the same
#ifdef HEAP_DEBUG
	KASSERT(footer == old_last_footer);
#endif

	//add the new merged chunk in the bucket list
	add_to_bucket_list(heap, prev);
	heap->number_of_chunks -= 2;
}

 static bool split_chunk_and_alloc(Heap* heap, HeapChunkHeader* header, uint64_t first_chunk_size){
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));
	KASSERT(first_chunk_size % 8 == 0);
	KASSERT(first_chunk_size >= HEAP_CHUNK_MIN_SIZE);

	//keep a copy of the old footer to check later if it's at the same address as the second_chunk_footer
#ifdef HEAP_DEBUG
	HeapChunkFooter* old_footer = get_heap_chunk_footer_from_header(header);
#endif

	int64_t second_chunk_size = get_fixed_heap_chunk_size(header);
	second_chunk_size -= first_chunk_size + sizeof(HeapChunkFooter) + sizeof(HeapChunkHeader);
	if(second_chunk_size < HEAP_CHUNK_MIN_SIZE)
		return false;
	KASSERT(second_chunk_size % 8 == 0)

	bool is_header_wilderness_chunk = get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK);
	remove_from_bucket_list(heap, header);

	//create the first chunk
	header->size = first_chunk_size;//do not set the free chunk flag, also do not the set the wilderness flag, if needed it will be added to the last
	HeapChunkFooter* first_chunk_footer = get_heap_chunk_footer_from_header(header);
	first_chunk_footer->size = first_chunk_size;
	//do not add to the bucket because it will not be free

	//create the second_chunk(the free one)
	//the next chunk ptr is caluculated from the data from the header, so it's possibile to just write this
	HeapChunkHeader* second_chunk = get_next_heap_chunk_header(header);
	second_chunk->size = (uint64_t)second_chunk_size;
	set_heap_chunk_flag(second_chunk, HEAP_FREE_CHUNK);
	if(is_header_wilderness_chunk){
		set_heap_chunk_flag(second_chunk, HEAP_WILDERNESS_CHUNK);
		heap->wilderness_chunk = second_chunk;
	}
	HeapChunkFooter* second_chunk_footer = get_heap_chunk_footer_from_header(second_chunk);
	second_chunk_footer->size = (uint64_t)second_chunk_size;
	add_to_bucket_list(heap, second_chunk);

#ifdef HEAP_DEBUG
	KASSERT(second_chunk_footer == old_footer);
#endif

	heap->number_of_chunks++;
	return true;
}

static void remove_from_bucket_list(Heap* heap, HeapChunkHeader* header){
	int index = get_bucket_index_from_header(header);
	HeapChunkHeader* prev_list_elem = get_bucket_list_prev(header);
	HeapChunkHeader* next_list_elem = get_bucket_list_next(header);
	KASSERT(get_heap_chunk_flag(header, HEAP_FREE_CHUNK));

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

static void add_to_bucket_list(Heap* heap, HeapChunkHeader* header){
	int index = get_bucket_index_from_header(header);
	set_bucket_list_prev(header, NULL);
	set_bucket_list_next(header, heap->free_buckets[index]);
	if(heap->free_buckets[index] != NULL)
		set_bucket_list_prev(heap->free_buckets[index], header);
	heap->free_buckets[index] = header;
}

static int get_bucket_index_from_size(uint64_t size){
	const uint64_t max_first = 8*64;
	const int max_index_first = 63;
	if(size <= max_first){
		int idx = (size/8)-1;//there cannot be a 8 byte bin because the minimum size is 16
		return idx;
	}

	const uint64_t max_second = max_first+(64*32);
	const int max_index_second = max_index_first + 32;
	if(size <= max_second){
		uint64_t idx = max_index_first+1;
		idx += (size-max_first) / 64;
		return idx;
	}

	uint64_t index = max_index_second+1;
	index += (size-max_second) / 512;
	if(index >= BUCKETS_COUNT)
		index = BUCKETS_COUNT-1;
	//the last bucket has 18K plus chunks

	return index;
}

static int get_bucket_index_from_header(HeapChunkHeader* header){
	//if it's the wild chunk return the last
	if(get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK))
		return BUCKETS_COUNT-1;
	return get_bucket_index_from_size(get_fixed_heap_chunk_size(header));
}

static void alloc_chunk(Heap* heap, HeapChunkHeader* header){
	KASSERT(!get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK));
	remove_from_bucket_list(heap, header);
	reset_heap_chunk_flag(header, HEAP_FREE_CHUNK);
}

bool kheap_initialized = false;

bool is_kheap_initialzed(){
	return kheap_initialized;
}

Heap kheap;
void kheap_init(void* start_heap_addr, uint64_t size){
	initizialize_heap(&kheap, start_heap_addr, size);
	kheap_initialized = true;
}

void* kmalloc(size_t size){
	if(size < HEAP_CHUNK_MIN_SIZE)
		size = HEAP_CHUNK_MIN_SIZE;

	if(size % 8 != 0)
		size += 8-(size%8);//align to 8 byte
	
	HeapChunkHeader* found_chunk = NULL;
	int bucket_idx = get_bucket_index_from_size(size);

	//search a suitable chunk in the bucket
	while(found_chunk == NULL && bucket_idx < BUCKETS_COUNT){
		for(HeapChunkHeader* candidate = kheap.free_buckets[bucket_idx]; candidate != NULL && found_chunk == NULL; candidate = get_bucket_list_next(candidate)){
			uint64_t candidate_size = get_fixed_heap_chunk_size(candidate);
			//check if the chunk can be splitted
			bool splittable = candidate_size >= HEAP_CHUNK_MIN_SIZE + HEAP_CHUNK_SPLIT_OVERHEAD;
			if(candidate_size == size || splittable){
				found_chunk = candidate;
			}
		}
		bucket_idx++;
	}

	//if there isn't a suitable chunk
	if(found_chunk == NULL){
		found_chunk = kheap.wilderness_chunk;
	}

	if(get_fixed_heap_chunk_size(found_chunk) == size)
		alloc_chunk(&kheap, found_chunk);
	else{
		//if it's not possibile to split it then the wilderness chunk isn't big enough
		if(!split_chunk_and_alloc(&kheap, found_chunk, size))
			kpanic(HEAP_OUT_OF_MEM);
	}
	
	return get_heap_chunk_user_data(found_chunk);
}

void kfree(void* ptr){
	HeapChunkHeader* header = get_heap_chunk_from_user_data(ptr);
	//cannot be free or wilderness chunk
	if(get_heap_chunk_flag(header, HEAP_FREE_CHUNK))
		kpanic(HEAP_CORRUPTED);
	if(get_heap_chunk_flag(header, HEAP_WILDERNESS_CHUNK))
		kpanic(HEAP_CORRUPTED);
	if(is_heap_chunk_corrupted(header))
		kpanic(HEAP_CORRUPTED);
	
	//free the chunk
	set_heap_chunk_flag(header, HEAP_FREE_CHUNK);
	set_bucket_list_next(header, NULL);
	set_bucket_list_prev(header, NULL);

	//check the next and prev chunk
	HeapChunkHeader *prev = NULL;
	HeapChunkHeader *next = NULL;

	if(!is_first_chunk_of_heap(&kheap, header))
		prev = get_prev_heap_chunk_header(header);

	if(!is_last_chunk_of_heap(header))
		next = get_next_heap_chunk_header(header);

	if(prev != NULL)
		if(!get_heap_chunk_flag(prev, HEAP_FREE_CHUNK))
			prev = NULL;

	if(next != NULL)
		if(!get_heap_chunk_flag(next, HEAP_FREE_CHUNK))
			next = NULL;

	//if there arent free adjacent then add to bucket list, if not merge them
	if(prev == NULL && next == NULL)
		add_to_bucket_list(&kheap, header);
	else if(prev != NULL && next == NULL)
		merge_with_next_chunk(&kheap, prev);
	else if(prev == NULL && next != NULL)
		merge_with_next_chunk(&kheap, header);
	else //(prev != NULL && next != NULL)
		merge_with_next_and_prev_chunk(&kheap, header);

	return;
}

uint64_t get_number_of_chunks_of_kheap(){
	return kheap.number_of_chunks;
}
