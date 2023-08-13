#pragma once
#include <stdbool.h>
#include "hal.h"
/*
this code reads the data in the elf headers(only the 64 bit version)
i tried to respect the ufficial name of the fields in the structures, so if you need to understand
what this code does it is recommended to read the wikipedia page (https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
or the ufficial documentation on the ELF(Executable and Linkable Format)
*/

enum ElfProgramHeaderType{
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,//reserved
	PT_PHDR = 6,
	PT_TLS = 7,
	PT_LOOS = 0x60000000,
	PT_HIOS = 0x6FFFFFFF,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7FFFFFFF
};

enum ElfSegmentFlags{
	PF_X = 1,
	PF_W = 2,
	PF_R = 4
};

typedef struct ElfHeader {
	char ei_magic[4];
	uint8_t ei_class;
	uint8_t ei_data;
	uint8_t ei_version;
	uint8_t ei_osabi;
	uint8_t ei_abiversion;
	uint8_t ei_pad[7];//unused
	uint16_t ei_type;
	uint16_t ei_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;

}__attribute__((packed)) ElfHeader;

typedef struct ElfProgramHeader {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
}__attribute__((packed)) ElfProgramHeader;

//simplified structure to represent the PT_LOAD segments(that has been loaded by the bootloader)
typedef struct ElfLoadedSegment {
	bool executable;
	bool writeable;
	bool readable;
	void* vaddr;//virtual address
	uint64_t size;
} ElfLoadedSegment;

bool elf_validate_header(ElfHeader* header);
uint16_t elf_get_number_of_loaded_entries(ElfHeader* header);
void elf_get_loaded_entries(ElfHeader* header, ElfLoadedSegment* out_buffer, size_t buffer_size);
bool elf_check_structs_size();
