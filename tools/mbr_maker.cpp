#include <iostream>
#include <cassert>
#include <memory.h>
#include <fstream>
#include <limits>

#define FAT32LBA_TYPE 0x0C

struct CHS{
	uint8_t C;
	uint8_t H;
	uint8_t S;
}__attribute__((packed));

struct PartitionTableEntry{
	uint8_t drive_attributes;
	CHS chs_start;
	uint8_t partition_type;
	CHS chs_last;
	uint32_t lba_start;
	uint32_t number_of_sectors;
}__attribute__((packed));

struct MBR{
	unsigned char bootloader_code[440];//Unique disk ID and reserved are ommited
	uint16_t disk_signature;
	uint32_t reserved;
	PartitionTableEntry partition_entries[4];
	unsigned char magic_number[2];
}__attribute__((packed));

void initMBR(MBR* in){
	in->magic_number[0] =0x55;
	in->magic_number[1] =0xAA;
	in->reserved = 0;
	return;
}

CHS lbaToChs(uint32_t lba){
	CHS out;
	const uint32_t heads_per_ciliders = 16;
	const uint32_t sectors_per_track = 63;
	out.C = lba / (heads_per_ciliders*sectors_per_track);
	out.H = (lba / sectors_per_track) % heads_per_ciliders;
	out.S = (lba % sectors_per_track) +1;
	return out;
}

PartitionTableEntry createPartitionTableEntry(bool bootable, uint32_t first_lba_sector, uint32_t size_in_sectors, uint8_t type){
	PartitionTableEntry out;
	if(bootable)
		out.drive_attributes = 0x80;//bootable
	else
		out.drive_attributes = 0x00;//inactive

	out.lba_start = first_lba_sector;//LBA
	out.number_of_sectors = size_in_sectors;//LBA
	out.chs_start = lbaToChs(first_lba_sector);//CHS
	out.chs_last = lbaToChs(first_lba_sector + size_in_sectors);//CHS
	out.partition_type = type;
	return out;
}

PartitionTableEntry createDummyPartitionTableEntry(){
	PartitionTableEntry out;
	memset(&out, 0, sizeof(out));
	out.drive_attributes = 0x7F;//invalid partition
	return out;
}

char* readBootloaderImage(std::string path, uint32_t* out_size){
	std::ifstream bin_image(path, std::ios::binary | std::ios::in);
	assert(bin_image.is_open());
	//get size of the file
	bin_image.seekg( 0, std::ios::end );
	uint32_t raw_size = bin_image.tellg();
	*out_size = raw_size;
	bin_image.seekg(0, std::ios::beg);

	//alloc buffer
	uint32_t buffer_size = raw_size;
	char* buffer = reinterpret_cast<char*>(malloc(buffer_size));
	memset(buffer, 0, buffer_size); 

	//read data
	bin_image.read(buffer, buffer_size);
	assert(!bin_image.bad());
	return buffer;
}


int main(int argc, char** argv){
	static_assert(sizeof(PartitionTableEntry) == 16, "PartitionTableEntry != 16!!");
	static_assert(sizeof(MBR) == 512, "MBR != 512!!");

	if(argc != 4){
		std::cerr << "usage: " << argv[0] << " <mbr_iso_file> <fat32 partition size(in sectors)> <binary_bootloader_image>" << std::endl;
		return -1;
	}

	uint32_t fat32_size_in_sectors = atoi(argv[2]);
	std::string mbr_path(argv[1]);
	std::string bootloader_path(argv[3]);

	//read code
	uint32_t bootloader_code_size;
	char* bootloader_code = readBootloaderImage(bootloader_path, &bootloader_code_size);

	//construct the partial image of the disk
	//create buffer for the partial image
	uint32_t partial_image_size_in_sectors = (bootloader_code_size-sizeof(MBR::bootloader_code));//size in bytes
	std::cerr << "bootloader code size in bytes: " << bootloader_code_size << std::endl;
	partial_image_size_in_sectors = (partial_image_size_in_sectors / 512) + ((partial_image_size_in_sectors % 512) != 0) +1;//convert to sectors with round up, +1 for the MBR
	std::cerr << "sector used for the bootloader code(mbr included): " << partial_image_size_in_sectors << std::endl;

	uint32_t real_partial_image_buffer_size = partial_image_size_in_sectors*512;
	char* partial_image_buffer = reinterpret_cast<char*>(malloc(real_partial_image_buffer_size));

	std::cerr << "it will be printed the first FAT32 sector(aka VBR) on stdout, all the previous prints were done on stderr(so you can remove it): " << std::endl;
	std::cout << partial_image_size_in_sectors << std::endl;

	//set up the MBR
	MBR* mbr_ptr = reinterpret_cast<MBR*>(partial_image_buffer);
	initMBR(mbr_ptr);
	mbr_ptr->disk_signature = rand() % std::numeric_limits<typeof(MBR::disk_signature)>::max();
	memcpy(mbr_ptr->bootloader_code, bootloader_code, sizeof(MBR::bootloader_code));//copy the first part of the bootloader code
	mbr_ptr->partition_entries[0] = createPartitionTableEntry(true, partial_image_size_in_sectors, fat32_size_in_sectors, FAT32LBA_TYPE);//fat32 partition, first sector after the space reserved to the second part of the bootloader
	for(int i = 1; i < 4; i++)
		mbr_ptr->partition_entries[i] = createDummyPartitionTableEntry();

	//copy the rest of the code
	//DO NOT USE THE memcpy function, the compiler will use SSE instructions on unaligned data and crash if you do
	for(int i = 0; i < bootloader_code_size-sizeof(MBR::bootloader_code); i++){
		assert(512+i < real_partial_image_buffer_size);
		assert(sizeof(MBR::bootloader_code)+i < bootloader_code_size);
		*(partial_image_buffer+512+i) = *(bootloader_code+sizeof(MBR::bootloader_code)+i);
	}

	//save on file
	std::ofstream mbr_file(mbr_path, std::ios::binary | std::ios::out);
	mbr_file.write(partial_image_buffer, real_partial_image_buffer_size);

	//fill extra data
    char temp_data[512];
	memset(temp_data, 0xcc, sizeof(temp_data));
	for (int i = 0; i < fat32_size_in_sectors; ++i){
    	mbr_file.write(temp_data, sizeof(temp_data));
	}

	free(bootloader_code);
	free(partial_image_buffer);
	return 0;
}
