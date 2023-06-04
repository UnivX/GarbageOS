#include "x64-memory.h"
#include "../../hal.h"
#include "../../memory.h"
#include "../../frame_allocator.h"

/*
 * internal version of the mmap function with a generic allocator
 */
void mmap(volatile void* vaddr, volatile void* paddr, uint8_t flags){
	uint64_t paddr_i = (uint64_t)paddr;
	uint64_t vaddr_i = (uint64_t)vaddr;

	flags |= PAGE_PRESENT;
	paddr_i &= 0x0000fffffffff000;

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
	PT[PT_index] = paddr_i | flags;

	asm("invlpg (%0)" : : "r"(vaddr));
	return;
}

void identity_map_memory(){

}
