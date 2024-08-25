#include "x64-memory.h"
#include "../../hal.h"
#include "../../mem/frame_allocator.h"
#include "../../util/sync_types.h"

typedef struct x64PagingStructure{
	volatile void* paging_structure;
	spinlock lock;
} x64PagingStructure;

inline void memory_fence(){
	asm volatile("mfence" :::"memory");
}

/*get the page table entry relative to the virtual address
 *Paging structure:
 *PML4[512] -> PDPT[512] -> PDT[512] -> PT[512]
 */

void enable_PAT(){
	uint64_t value = PAT_VALUE;
	uint32_t low = value;
	uint32_t high = value >> 32;
	set_cpu_msr(IA32_PAT_MSR, low, high);
	//flush TLB
	uint64_t cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3) : : "cc");
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : "cc");
}

void set_active_paging_structure(volatile void* _paging_structure){
	volatile x64PagingStructure* x64_paging_struct = (volatile x64PagingStructure*)_paging_structure;
	volatile void* paging_structure = x64_paging_struct->paging_structure;
	uint64_t cr3 = (uint64_t)paging_structure;
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : "cc");
}

void* get_bootloader_paging_structure(){
	uint64_t cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3) : : "cc");
	cr3 &= 0xfffffffffffff000;

	x64PagingStructure* x64paging_structure = (x64PagingStructure*)alloc_frame();
	init_spinlock(&x64paging_structure->lock);
	x64paging_structure->paging_structure = (void*)cr3;
	return (void*)x64paging_structure;
}

volatile void* get_raw_paging_structure(void* paging_structure){
	x64PagingStructure* x64_paging_struct = (x64PagingStructure*)paging_structure;
	return x64_paging_struct->paging_structure;
}

uint64_t* unsync_get_page_table_entry(volatile void* paging_structure, volatile void* vaddr){
	uint64_t vaddr_i = (uint64_t)vaddr;

	//9 bits each
	uint16_t PML4_index = (vaddr_i >> 39) & 0x1ff;
	uint16_t PDPT_index = (vaddr_i >> 30) & 0x1ff;
	uint16_t PDT_index = (vaddr_i >> 21) & 0x1ff;
	uint16_t PT_index = (vaddr_i >> 12) & 0x1ff;

	uint64_t* PML4 = (uint64_t*)paging_structure;
	if((PML4[PML4_index] & PAGE_PRESENT)== 0){
		uint64_t new_frame = (uint64_t)alloc_frame()&0x0000fffffffff000;
		memset((void*)new_frame, 0, PAGE_SIZE);
		PML4[PML4_index] =  new_frame | PAGE_PRESENT | PAGE_WRITABLE;
	}
	uint64_t* PDPT = (uint64_t*)(PML4[PML4_index]&0x0000fffffffff000);

	if((PDPT[PDPT_index] & PAGE_PRESENT) == 0){
		uint64_t new_frame = (uint64_t)alloc_frame()&0x0000fffffffff000;
		memset((void*)new_frame, 0, PAGE_SIZE);
		PDPT[PDPT_index] =  new_frame | PAGE_PRESENT | PAGE_WRITABLE;
	}
	uint64_t* PDT = (uint64_t*)(PDPT[PDPT_index]&0x0000fffffffff000);

	if((PDT[PDT_index] & PAGE_PRESENT) == 0){
		uint64_t new_frame = (uint64_t)alloc_frame()&0x0000fffffffff000;
		memset((void*)new_frame, 0, PAGE_SIZE);
		PDT[PDT_index] =  new_frame | PAGE_PRESENT | PAGE_WRITABLE;
	}
	uint64_t* PT = (uint64_t*)(PDT[PDT_index]&0x0000fffffffff000);
	return &(PT[PT_index]);
}

//ivalidate the MMU cache of relative to the virtual address received as parameter
static inline void invalidate_TLB(volatile void* vaddr){
	memory_fence();
	asm volatile("invlpg (%0)" : : "r"(vaddr));
}

void delete_page_translation_cache(volatile void* virtual_addr){
	invalidate_TLB(virtual_addr);
}

void delete_all_translation_cache(){
	uint64_t cr3;
	asm volatile("mov %%cr3, %0" : "=r"(cr3) : : "cc");
	asm volatile("mov %0, %%cr3" : : "r"(cr3) : "cc");
}

void paging_map(volatile void* _paging_structure, volatile void* vaddr, volatile void* paddr, uint16_t flags){
	volatile x64PagingStructure* x64_paging_struct = (volatile x64PagingStructure*)_paging_structure;
	//ACQUIRE_SPINLOCK_HARD(&x64_paging_struct->lock);
	uint64_t* table_entry = unsync_get_page_table_entry(x64_paging_struct->paging_structure, vaddr);

	flags |= PAGE_PRESENT;
	flags &= 0xfff;

	uint64_t paddr_i = (uint64_t)paddr;
	paddr_i &= 0x0000fffffffff000;

	*table_entry = paddr_i | flags;
	//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
	invalidate_TLB(vaddr);
}

void invalidate_paging_mapping(volatile void* _paging_structure, volatile void* vaddr){
	volatile x64PagingStructure* x64_paging_struct = (volatile x64PagingStructure*)_paging_structure;
	uint64_t* table_entry = unsync_get_page_table_entry(x64_paging_struct->paging_structure, vaddr);
	uint64_t new_value = *table_entry;
	new_value &= ~(uint64_t)(PAGE_PRESENT);//disable PAGE_PRESENT bit
	*table_entry = new_value;
	invalidate_TLB(vaddr);
}

PagingMapState get_physical_address(volatile void* _paging_structure, volatile void* vaddr){
	/* NOTE:
	 * because the retrive of the physical address doesn't needs
	 * to allocate eventualy needed frames for the paging structure
	 */
	PagingMapState mmap_not_present;
	mmap_not_present.flags = 0;
	mmap_not_present.paddr = 0;
	mmap_not_present.mmap_present = false;
	uint64_t vaddr_i = (uint64_t)vaddr;
	//9 bits each
	uint16_t PML4_index = (vaddr_i >> 39) & 0x1ff;
	uint16_t PDPT_index = (vaddr_i >> 30) & 0x1ff;
	uint16_t PDT_index = (vaddr_i >> 21) & 0x1ff;
	uint16_t PT_index = (vaddr_i >> 12) & 0x1ff;

	volatile x64PagingStructure* x64_paging_struct = (volatile x64PagingStructure*)_paging_structure;


	//ACQUIRE_SPINLOCK_HARD(&x64_paging_struct->lock);
	uint64_t* PML4 = (uint64_t*)x64_paging_struct->paging_structure;
	if((PML4[PML4_index] & PAGE_PRESENT)== 0){
		//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
		return mmap_not_present;
	}

	uint64_t* PDPT = (uint64_t*)(PML4[PML4_index]&0x0000fffffffff000);

	if((PDPT[PDPT_index] & PAGE_PRESENT) == 0){
		//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
		return mmap_not_present;
	}

	uint64_t* PDT = (uint64_t*)(PDPT[PDPT_index]&0x0000fffffffff000);

	if((PDT[PDT_index] & PAGE_PRESENT) == 0){
		//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
		return mmap_not_present;
	}

	uint64_t* PT = (uint64_t*)(PDT[PDT_index]&0x0000fffffffff000);

	uint64_t page_table_entry = PT[PT_index];
	uint16_t flags = page_table_entry & 0xfff;
	//CANONICAL ADDRESS
	//if the last used bit is 1 make the rest of the address 1
	uint64_t paddr_i = page_table_entry & 0x0000fffffffff000;
	if((paddr_i & 0x0000800000000000) != 0){
		paddr_i |= 0xffff000000000000;
	}
	if((flags & PAGE_PRESENT) == 0){
		mmap_not_present.paddr = (void*)paddr_i;
		mmap_not_present.flags = flags;
		//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
		return mmap_not_present;
	}

	//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
	PagingMapState result = {(void*)paddr_i, flags, true};
	return result;
}

void* create_empty_kernel_paging_structure(){
	uint64_t* paging_structure = alloc_frame();
	memset(paging_structure, 0, PAGE_SIZE);

	for(int i = 0; i < 512; i++){
		uint64_t new_frame = (uint64_t)alloc_frame()&0x0000fffffffff000;
		memset((void*)new_frame, 0, PAGE_SIZE);
		paging_structure[i] =  new_frame | PAGE_PRESENT | PAGE_WRITABLE;
	}

	x64PagingStructure* x64paging_structure = (x64PagingStructure*)alloc_frame();
	init_spinlock(&x64paging_structure->lock);
	x64paging_structure->paging_structure = paging_structure;
	return (void*)x64paging_structure;
}

void delete_map_table(uint64_t* map_table, uint8_t page_level){
	//PML4 	== 4
	//PDPT 	== 3
	//PDT  	== 2
	//PT 	== 1
	if(page_level == 0)
		return;
	for(uint16_t i = 0; i < 512 && page_level != 1; i++){
		if((map_table[i] & PAGE_PRESENT) != 0){
			uint64_t* inner_map_table = (uint64_t*)(map_table[i]&0x0000fffffffff000);
			delete_map_table(inner_map_table, page_level-1);
		}
	}
	dealloc_frame((void*)map_table);
}

void delete_paging_structure(void* _paging_structure){
	return;
	x64PagingStructure* x64_paging_struct = (x64PagingStructure*)_paging_structure;
	delete_map_table((uint64_t*)x64_paging_struct->paging_structure, 4);
	dealloc_frame((void*)x64_paging_struct);
}

void copy_paging_structure_mapping_no_page_invalidation(void* _src_paging_structure, void* _dst_paging_structure, void* start_vaddr, uint64_t size_bytes, uint16_t new_flags){
	void* vaddr = start_vaddr;
	x64PagingStructure* x64_src_paging_struct = (x64PagingStructure*)_src_paging_structure;
	x64PagingStructure* x64_dst_paging_struct = (x64PagingStructure*)_dst_paging_structure;
	for(uint64_t i = 0; i < size_bytes; i += PAGE_SIZE){
		//ACQUIRE_SPINLOCK_HARD(&x64_src_paging_struct->lock);
		//acquire_spinlock(&x64_dst_paging_struct->lock);

		uint64_t* src_entry = unsync_get_page_table_entry(x64_src_paging_struct->paging_structure, vaddr);
		uint64_t* dst_entry = unsync_get_page_table_entry(x64_dst_paging_struct->paging_structure, vaddr);
		if(new_flags == 0)
			*dst_entry = *src_entry;
		else
			*dst_entry = (*src_entry & 0x0000fffffffff000) | new_flags;
		vaddr += PAGE_SIZE;

		//release_spinlock(&x64_dst_paging_struct->lock);
		//RELEASE_SPINLOCK_HARD(&x64_src_paging_struct->lock);
	}
}

void count_map_tables_frames(uint64_t* map_table, uint8_t page_level, uint64_t* counter){
	//PML4 	== 4
	//PDPT 	== 3
	//PDT  	== 2
	//PT 	== 1

	if(page_level == 0)
		return;
	for(uint16_t i = 0; i < 512 && page_level != 1; i++){
		if((map_table[i] & PAGE_PRESENT) != 0){
			uint64_t* inner_map_table = (uint64_t*)(map_table[i]&0x0000fffffffff000);
			count_map_tables_frames(inner_map_table, page_level-1, counter);
		}
	}
	(*counter)++;
}

uint64_t get_paging_mem_overhead(void* _paging_structure){
	x64PagingStructure* x64_paging_struct = (x64PagingStructure*)_paging_structure;
	uint64_t counter = 0;
	//ACQUIRE_SPINLOCK_HARD(&x64_paging_struct->lock);
	count_map_tables_frames((void*)x64_paging_struct->paging_structure, 4, &counter);
	//RELEASE_SPINLOCK_HARD(&x64_paging_struct->lock);
	return counter*PAGE_SIZE;
}

