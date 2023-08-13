//heap implementation
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../hal.h"
#include "../kdefs.h"

#define HEAP_FREE_CHUNK 1
#define HEAP_WILDERNESS_CHUNK 2
#define HEAP_FLAG3 4
#define BUCKETS_COUNT 128

//TODO:
//-function for merging chunks
//-function for splitting chunks
//-all the other stuff

/*
 *
 * Heap struct:
 *
 * HEADER CHUNK 0
 * USER DATA
 * FOOTER CHUNK 0
 * HEADER CHUNK 1
 * USER DATA
 * FOOTER CHUNK 1
 * ...
 * WILDERNESS CHUNK (it's size can grow with the heap, so it is considered to have unlimited memory)
 *
 *
 * when the chunk is free the space for the user data is used to store the pointer to the next
 * chunk in the bucket, so a chunk must be at least 8 bytes long
 */

//it's 8 byte aligned so the first 3 bit are alway zero and can be used as flags
typedef struct HeapChunkHeader{
	uint64_t size;//the size is only that of the user data
} HeapChunkHeader;

static uint64_t get_fixed_heap_chunk_size(HeapChunkHeader* header);
static bool get_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag);
static void set_heap_chunk_flag(HeapChunkHeader* header, uint8_t flag);
static HeapChunkHeader* get_next_heap_chunk_header(HeapChunkHeader* header);
static HeapChunkHeader* get_prev_heap_chunk_header(HeapChunkHeader* header);
static void* get_heap_chunk_user_data(HeapChunkHeader* header);
static void set_bucket_list_next(HeapChunkHeader* header, HeapChunkHeader* next);
static HeapChunkHeader* get_bucket_list_next(HeapChunkHeader* header);
static bool is_heap_chunk_corrupted(HeapChunkHeader* header);

typedef struct HeapChunkFooter{
	uint64_t size;//at the moment it doesnt store any flag
}HeapChunkFooter;

static uint64_t get_fixed_heap_chunk_size_from_footer(HeapChunkFooter* footer);
static HeapChunkHeader* get_heap_chunk_header_from_footer(HeapChunkFooter* footer);
static HeapChunkFooter* get_heap_chunk_footer_from_header(HeapChunkHeader* header);

typedef struct Heap{
	bool enable_growth;//flag that enables the heap to grow when needed
	void* start;
	uint64_t size;
	HeapChunkHeader* wilderness_chunk;
	HeapChunkHeader* free_buckets[BUCKETS_COUNT];//multiple linked list of free chunks, each bucket has its size
} Heap;

static void initizialize_heap(Heap* heap, void* start, uint64_t size);
static void enable_heap_growth(Heap* heap);
static bool is_first_chunk_of_heap(Heap* heap, HeapChunkHeader* header);
static bool is_last_chunk_of_heap(HeapChunkHeader* header);
