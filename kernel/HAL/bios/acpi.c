#include "../../hal.h"
#include "../../kio.h"
#include "../../mem/vmm.h"

//get the RSDP on bios systems
void* acpi_RSDP(){
	//from the ACPI specification says that on IA-PC systems the RSDP can be found in the first KB of memory and between 0x0E0000 and 0x0FFFFF
	//get readonly memory first KB of EBDA
	//the EBDA has a pointer at address 0x40E
	VMemHandle first_page = memory_map((void*)0, PAGE_SIZE, PAGE_PRESENT);//readonly
	volatile uint16_t EBDA_segment = *((volatile uint16_t*)(get_vmem_addr(first_page) + 0x40E));
	deallocate_kernel_virtual_memory(first_page);
	void* EBDA_addr = (void*)(((uint64_t)EBDA_segment*0x10) & 0x000FFFFF);
	//align to PAGE_BOUNDARY
	uint64_t EBDA_virtual_mem_offset = ((uint64_t)EBDA_addr%PAGE_SIZE);//offset from the start of the virtual memory
	void* aligned_EBDA_addr = EBDA_addr - ((uint64_t)EBDA_addr%PAGE_SIZE);
	//0E0000h and 0FFFFFh.
	uint64_t BIOS_RO_start 	= 0x0E0000;
	uint64_t BIOS_RO_end 	= 0x0FFFFF;
	uint64_t BIOS_RO_size = BIOS_RO_end+1-BIOS_RO_start;
	void* RSDP = NULL;

	char string_signature[] = "RSD PTR ";
	uint64_t signature = *(uint64_t*)string_signature;

	//16 byte boundaries
	VMemHandle first_kb = memory_map(aligned_EBDA_addr, 2*PAGE_SIZE, PAGE_PRESENT);//readonly, use two pages to be sure
	for(uint64_t offset = 0; offset < 1*KB && RSDP == NULL; offset += 16){
		uint64_t* addr = (uint64_t*)(get_vmem_addr(first_kb) + offset + EBDA_virtual_mem_offset - (EBDA_virtual_mem_offset%16));
		if(*addr == signature)
			RSDP = (void*)((uint64_t)addr-(uint64_t)get_vmem_addr(first_kb) + (uint64_t)EBDA_addr);
	}
	deallocate_kernel_virtual_memory(first_kb);

	//16 byte boundaries
	VMemHandle BIOS_readonly = memory_map((void*)0x0E0000, BIOS_RO_size, PAGE_PRESENT);//readonly
	for(uint64_t offset = 0; offset < BIOS_RO_size && RSDP == NULL; offset += 16){
		uint64_t* addr = (uint64_t*)(get_vmem_addr(BIOS_readonly) + offset);
		if(*addr == signature)
			RSDP = (void*)((uint64_t)addr-(uint64_t)get_vmem_addr(BIOS_readonly)+BIOS_RO_start);
	}
	deallocate_kernel_virtual_memory(BIOS_readonly);

	return RSDP;
}
