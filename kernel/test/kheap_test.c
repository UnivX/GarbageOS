#include "kheap_test.h"

void heap_stress_test(){
	uint64_t start_number_of_chunks = get_number_of_chunks_of_kheap();
	const int max = 100;
	void* allocated[max];
	size_t sizes[5] = {32, 34, 512, 129, 16};
	int sizes_idx = 0;

	for(int i = 0; i < max; i++){
		allocated[i] = kmalloc(sizes[sizes_idx]);
		sizes_idx = (sizes_idx+1) % 5;
	}

	int n_of_free = 0;
	uint32_t rand = 0x79a9f11c;
	uint32_t times = 500;
	while(n_of_free < 100 && times > 0){
		uint32_t next_idx = rand % max;
		if(allocated[next_idx] != NULL){
			n_of_free++;
			kfree(allocated[next_idx]);
			allocated[next_idx] = NULL;
		}
		rand = (rand ^ ((0x672890f1+(~(rand ^ 0xb69f5cca) + n_of_free)) + rand-(~n_of_free) % (rand ^ 0x672890f1)));
		times--;
	}

	const size_t huge_mem_size = 7*MB;
	void* huge_memory_allocation = kmalloc(huge_mem_size);
	memset(huge_memory_allocation, 0xff, huge_mem_size);
	kfree(huge_memory_allocation);

	for(int i = 0; i < max; i++)
		if(allocated[i] != NULL)
			kfree(allocated[i]);

	bool heap_corrupted = is_kheap_corrupted();
	if(get_number_of_chunks_of_kheap() == start_number_of_chunks && !heap_corrupted)
		print("HEAP stress test PASSED\n");
	else{
		print("HEAP stress test FAILED / heap chunks from ");
		print_uint64_dec(get_number_of_chunks_of_kheap());
		print(" to ");
		print_uint64_dec(start_number_of_chunks);
		print("\n");
		print("HEAP IS ");
		if(!heap_corrupted)
			print("NOT ");
		print("CORRUPTED\n");
	}
}
