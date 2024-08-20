//heap implementation
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../hal.h"
#include "../kdefs.h"
#include "../util/sync_types.h"
#include "vmm.h"

#define HEAP_FREE_CHUNK 1
#define HEAP_WILDERNESS_CHUNK 2
#define HEAP_FLAG3 4
#define BUCKETS_COUNT 128
#define HEAP_CHUNK_MIN_SIZE 16
#define HEAP_ALIGNMENT 8
#define HEAP_DEBUG
#define HEAP_CHUNK_SPLIT_OVERHEAD sizeof(HeapChunkHeader) + sizeof(HeapChunkFooter)

//TODO:
//-make the size of the footer and header 32 bit and rework the initial big chunk in initialization
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
//the chunk must be at least 16 byte

typedef struct HeapChunkFooter{
	uint64_t size;//at the moment it doesnt store any flag
}HeapChunkFooter;

typedef struct Heap{
	VMemHandle heap_vmem;
	bool enable_growth;//flag that enables the heap to grow when needed
	void* start;
	uint64_t size;
	uint64_t number_of_chunks;
	HeapChunkHeader* wilderness_chunk;
	HeapChunkHeader* free_buckets[BUCKETS_COUNT];//multiple linked list of free chunks, each bucket has its size
} Heap;

//if not splittable return false
//the requested chunk will be that pointed by header argument
//the spare one will be the next to the requested chunk

bool is_kheap_initialzed();
void kheap_init();
void* kmalloc(size_t size);
void kfree(void* ptr);
uint64_t get_number_of_chunks_of_kheap();
bool is_kheap_corrupted();
uint64_t get_kheap_total_size();
void enable_kheap_growth();
