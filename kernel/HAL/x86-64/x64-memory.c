#include "x64-memory.h"
#include "../../hal.h"
#include "../../memory.h"
#include "../../frame_allocator.h"

/*get the page table entry relative to the virtual address
 *Paging structure:
 *PML4[512] -> PDPT[512] -> PDT[512] -> PT[512]
 */
uint64_t* get_page_table_entry(volatile void* vaddr){
	uint64_t vaddr_i = (uint64_t)vaddr;

	//9 bits each
	uint16_t PML4_index = (vaddr_i >> 39) & 0x1ff;
	uint16_t PDPT_index = (vaddr_i >> 30) & 0x1ff;
	uint16_t PDT_index = (vaddr_i >> 21) & 0x1ff;
	uint16_t PT_index = (vaddr_i >> 12) & 0x1ff;

	//get cr3
	uint64_t cr3;
	asm("mov %%cr3, %0" : "=r"(cr3));
	cr3 &= 0xfffffffffffff000;

	uint64_t* PML4 = (uint64_t*)cr3;
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
void invalidate_TLB(volatile void* vaddr){
	asm("invlpg (%0)" : : "r"(vaddr));
}

void mmap(volatile void* vaddr, volatile void* paddr, uint16_t flags){
	uint64_t* table_entry = get_page_table_entry(vaddr);

	flags |= PAGE_PRESENT;
	flags &= 0xfff;

	uint64_t paddr_i = (uint64_t)paddr;
	paddr_i &= 0x0000fffffffff000;

	*table_entry = paddr_i | flags;
	invalidate_TLB(vaddr);
}

void invalidate_mmap(volatile void* vaddr){
	uint64_t* table_entry = get_page_table_entry(vaddr);
	uint64_t new_value = *table_entry;
	new_value &= ~(uint64_t)(PAGE_PRESENT);//disable PAGE_PRESENT bit
	*table_entry = new_value;
	invalidate_TLB(vaddr);
}

PagingMapState get_physical_address(volatile void* vaddr){
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

	//get cr3
	uint64_t cr3;
	asm("mov %%cr3, %0" : "=r"(cr3));
	cr3 &= 0xfffffffffffff000;

	uint64_t* PML4 = (uint64_t*)cr3;
	if((PML4[PML4_index] & PAGE_PRESENT)== 0)
		return mmap_not_present;

	uint64_t* PDPT = (uint64_t*)(PML4[PML4_index]&0x0000fffffffff000);

	if((PDPT[PDPT_index] & PAGE_PRESENT) == 0)
		return mmap_not_present;

	uint64_t* PDT = (uint64_t*)(PDPT[PDPT_index]&0x0000fffffffff000);

	if((PDT[PDT_index] & PAGE_PRESENT) == 0)
		return mmap_not_present;

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
		return mmap_not_present;
	}
	PagingMapState result = {(void*)paddr_i, flags, true};
	return result;
}

void identity_map_memory(){
	FreePhysicalMemoryStruct memory = free_mem_bootloader();
	for(size_t i = 0; i < memory.number_of_ranges; i++){
		uint64_t start_addr = memory.free_ranges[i].start_address;
		uint64_t pages = memory.free_ranges[i].size / PAGE_SIZE;
		for(uint64_t j = 0; j < pages; j++){
			uint64_t paddr_i = start_addr + (j*PAGE_SIZE);
			mmap((void*)paddr_i, (void*)paddr_i, PAGE_WRITABLE);
		}
	}
}
