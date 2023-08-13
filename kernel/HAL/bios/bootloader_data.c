#include <stdbool.h>
#include "bootloader_data.h"
#include "../../hal.h"
#include "../../kdefs.h"

BootLoaderData* get_bootloader_data(){
	return *(BootLoaderData**)(0x0600);
}

/*
 * since this function will be called by the frame allocator we cannot allocate any type of dynamic memory
 * we will use a global variable as a buffer
 */
PhysicalMemoryRange free_range_buffer[MAX_PHYSICAL_MEMORY_RANGES];
int64_t free_ranges_number = -1;

FreePhysicalMemoryStruct free_mem_bootloader(){
	if(free_ranges_number != -1){
		FreePhysicalMemoryStruct free_memory;
		free_memory.free_ranges = free_range_buffer;
		free_memory.number_of_ranges = free_ranges_number;
		return free_memory;
	}

	BootLoaderData* boot_data = get_bootloader_data();
	int64_t next_range = 0;
	MemoryMapItem* map_items = boot_data->map_items;

	//fill out the range_buffer with the free ranges
	for(uint32_t i = 0; i < *boot_data->map_items_count; i++){
		if(map_items[i].type == MEMORY_MAP_FREE && map_items[i].size != 0){
			//the correct call of this function is MANDATORY for the rest of the kernel
			if(next_range == MAX_PHYSICAL_MEMORY_RANGES)
				kpanic(FREE_MEM_BOOTLOADER_ERROR);
			free_range_buffer[next_range].start_address = map_items[i].base_addr;
			free_range_buffer[next_range].size = map_items[i].size;
			next_range++;
		}
	}

	//sort the MemoryMapItem array with a bubblesort
	for(int i = 0; i < next_range; i++)
		for(int j = 0; j < next_range-i-1; j++)
			if(free_range_buffer[j].start_address > free_range_buffer[j+1].start_address){
				PhysicalMemoryRange temp = free_range_buffer[j];
				free_range_buffer[j] = free_range_buffer[j+1];
				free_range_buffer[j+1] = temp;
			}

	//remove the used frames by the bootloader frame allocator
	uint32_t frame_alloc_memory_map_offset = boot_data->frame_allocator_data->memory_map_item_offset;
	MemoryMapItem* frame_alloc_map_item = (MemoryMapItem*)((uint64_t)frame_alloc_memory_map_offset);

	for(int i = 0; i < next_range; i++){
		//NOTE: since the bootloader use only a single range of free memory we need to fix only that one
		if(frame_alloc_map_item->base_addr == free_range_buffer[i].start_address){
			//set the correct first_free_address and patch its size
			free_range_buffer[i].size -= boot_data->frame_allocator_data->first_frame_address-free_range_buffer[i].start_address;
			free_range_buffer[i].start_address = boot_data->frame_allocator_data->first_frame_address;
		}
	}

	//remove the first 1MB used by the bootloader
	for(int i = 0; i < next_range; i++){
		if(free_range_buffer[i].start_address < 1*MB){
			if(free_range_buffer[i].start_address+free_range_buffer[i].size > 1*MB){
				//if the range end after the 1MB then fix it
				free_range_buffer[i].size -= 1*MB-free_range_buffer[i].start_address;
				free_range_buffer[i].start_address = 1*MB;
			}else{
				//if the range is contained in the first 1MB then set its size to zero
				free_range_buffer[i].size = 0;
			}
		}
	}

	//align to 4k
	for(int i = 0; i < next_range; i++){
		if(free_range_buffer[i].size == 0) continue;
		//if the start address it's not aligned round it up to next page
		if(free_range_buffer[i].start_address % PAGE_SIZE != 0){
			uint64_t to_add = -(free_range_buffer[i].start_address % PAGE_SIZE) + PAGE_SIZE;
			free_range_buffer[i].start_address += to_add;
			free_range_buffer[i].size -= to_add;
		}
		//if the size is not aligned round it down
		if(free_range_buffer[i].size % PAGE_SIZE != 0){
			free_range_buffer[i].size -= free_range_buffer[i].size % PAGE_SIZE;
		}
	}
	
	
	FreePhysicalMemoryStruct free_memory;
	free_memory.free_ranges = free_range_buffer;
	free_memory.number_of_ranges = next_range;//next_range is used as a counter so we can use it to track the number of ranges
	free_ranges_number = free_memory.number_of_ranges;
	return free_memory;
}


uint64_t get_total_usable_RAM_size(){
	uint64_t total = 0;
	BootLoaderData* boot_data = get_bootloader_data();
	MemoryMapItem* map_items = boot_data->map_items;
	for(uint32_t i = 0; i < *boot_data->map_items_count; i++){
		if(map_items[i].type == MEMORY_MAP_FREE && map_items[i].size != 0){
			total += map_items[i].size;
		}
	}
	return total;
}

uint64_t get_last_address(){
	uint64_t last_addr = 0;

	BootLoaderData* boot_data = get_bootloader_data();
	MemoryMapItem* map_items = boot_data->map_items;

	for(uint32_t i = 0; i < *boot_data->map_items_count; i++){
		//if its valid
		if(map_items[i].size != 0){
			uint64_t map_item_last_addr = map_items[i].base_addr + map_items[i].size-REGISTER_SIZE_BYTES;
			if(map_item_last_addr > last_addr)
				last_addr = map_item_last_addr;
		}
	}
	return last_addr;
}

PhysicalMemoryRange get_bootstage_indentity_mapped_RAM(){
	//the value it's hardcoded, if the bootloader change the original identity_map then 
	//this value needs to be changed
	PhysicalMemoryRange result = {0,1*GB};
	return result;
}

void* get_kernel_image(){
	BootLoaderData* boot_data = get_bootloader_data();
	return boot_data->elf_image;
}
