#include "elf.h"

bool elf_check_structs_size(){
	bool ok = true;
	ok &= sizeof(ElfHeader) == 0x40;
	ok &= sizeof(ElfProgramHeader) == 0x38;
	return ok;
}

bool elf_validate_header(ElfHeader* header){
	bool ok = true;
	//check the magic number
	ok &= header->ei_magic[0] == 0x7f;
	ok &= header->ei_magic[1] == 'E';
	ok &= header->ei_magic[2] == 'L';
	ok &= header->ei_magic[3] == 'F';

	//the structure are 64 bit so it must be 64
	ok &= header->ei_class == 2;
	
	//check that the endianess is the same
	if(ENDIANESS == LITTLE_ENDIAN)
		ok &= header->ei_data == 1;
	if(ENDIANESS == BIG_ENDIAN)
		ok &= header->ei_data == 2;

	ok &= header->ei_version == 1;
	ok &= header->e_version == 1;
	return ok;
}

uint16_t elf_get_number_of_loaded_entries(ElfHeader* header){
	uint16_t counter = 0;
	ElfProgramHeader *pheader = (ElfProgramHeader*)(((void*)header)+header->e_phoff);
	for(uint16_t i = 0; i < header->e_phnum; i++){
		if(pheader->p_type == PT_LOAD)
			counter++;
		//jump to the next
		void* next_pheader = ((void*)pheader)+header->e_phentsize;
		pheader = (ElfProgramHeader*)next_pheader;
	}
	return counter;
}

//TODO: test the function
void elf_get_loaded_entries(ElfHeader* header, ElfLoadedSegment* out_buffer, size_t buffer_size){
	uint32_t buffer_index = 0;
	uint16_t ph_counter = 0;//program header counter
	ElfProgramHeader *pheader = (ElfProgramHeader*)(((void*)header)+header->e_phoff);

	while(ph_counter < header->e_phnum && buffer_index < buffer_size){
		if(pheader->p_type == PT_LOAD){
			ElfLoadedSegment new_segment;
			//fill the segment
			new_segment.executable = (pheader->p_flags & PF_X) != 0;
			new_segment.writeable = (pheader->p_flags & PF_W) != 0;
			new_segment.readable = (pheader->p_flags & PF_R) != 0;
			new_segment.vaddr = (void*)pheader->p_vaddr;
			new_segment.size = pheader->p_memsz;
			out_buffer[buffer_index] = new_segment;
			buffer_index++;
		}
	
		//jump to the next program header
		void* next_pheader = ((void*)pheader)+header->e_phentsize;
		pheader = (ElfProgramHeader*)next_pheader;
		ph_counter++;
	}
}
