//http://elm-chan.org/docs/fat_e.html#notes
#include <memory>
#include <iostream>
#include <filesystem>
#include <map>
#include <sstream>
#include <memory.h>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <time.h>

#define KB 1024
#define MB 1024 * KB
#define GB 1024 * MB

#define MIN_FAT32_CLUSTERS 65526

#define ACTIVE_FAT_FLAG(fat_number) (1 << (fat_number-1))

/*
FAT32 AREAS:
-RESERVED AREA
	-VOLUME BOOT SECTOR
	-FSINFO SECTOR
	-BACKUP SECTORS

-FAT AREA

-DATA AREA

*/

#define ATTR_DIRECTORY 0x10
#define FAT_ENTRY_END_OF_CHAIN 0x0FFFFFFF
#define DIRECTORY_ENTRY_FREE 0xE5
#define DIRECTORY_ENTRY_FREE_AND_AFTER_FREE 0x00

//define naming convention: <FAT32 AREA>_<OFFSET or SIZE>_<FIELD NAME>
#define VBR_OFFSET_JMP_BOOT 0
#define VBR_SIZE_JMP_BOOT 3
#define VBR_OFFSET_OEM_NAME 3
#define VBR_SIZE_OEM_NAME 8
#define VBR_OFFSET_BYTS_PER_SEC 11
#define VBR_SIZE_BYTS_PER_SEC 2
#define VBR_OFFSET_SEC_PER_CLUS 13
#define VBR_SIZE_SEC_PER_CLUS 1
#define VBR_OFFSET_RSVD_SEC_CNT 14
#define VBR_SIZE_RSVD_SEC_CNT 2
#define VBR_OFFSET_NUM_FATS 16
#define VBR_SIZE_NUM_FATS 1
#define VBR_OFFSET_ROOT_ENT_CNT 17
#define VBR_SIZE_ROOT_ENT_CNT 2
#define VBR_OFFSET_TOT_SEC_16 19
#define VBR_SIZE_TOT_SEC_16 2
#define VBR_OFFSET_MEDIA 21
#define VBR_SIZE_MEDIA 1
#define VBR_OFFSET_FAT_SZ_16 22
#define VBR_SIZE_FAT_SZ_16 2
#define VBR_OFFSET_SEC_PER_TRK 24
#define VBR_SIZE_SEC_PER_TRK 2
#define VBR_OFFSET_NUM_HEADS 26
#define VBR_SIZE_NUM_HEADS 2
#define VBR_OFFSET_HIDD_SEC 28
#define VBR_SIZE_HIDD_SEC 4
#define VBR_OFFSET_TOT_SEC_32 32
#define VBR_SIZE_TOT_SEC_32 4
#define VBR_OFFSET_FAT_SZ_32 36
#define VBR_SIZE_FAT_SZ_32 4
#define VBR_OFFSET_EXT_FLAGS 40
#define VBR_SIZE_EXT_FLAGS 2
#define VBR_OFFSET_FS_VER 42
#define VBR_SIZE_FS_VER 2
#define VBR_OFFSET_ROOT_CLUS 44
#define VBR_SIZE_ROOT_CLUS 4
#define VBR_OFFSET_FS_INFO 48
#define VBR_SIZE_FS_INFO 2
#define VBR_OFFSET_BK_BOOT_SEC 50
#define VBR_SIZE_BK_BOOT_SEC 2
#define VBR_OFFSET_BPB_RESERVED 52
#define VBR_SIZE_BPB_RESERVED 12
#define VBR_OFFSET_DRV_NUM 64
#define VBR_SIZE_DRV_NUM 1
#define VBR_OFFSET_BS_RESERVED 65
#define VBR_SIZE_BS_RESERVED 1
#define VBR_OFFSET_BOOT_SIG 66
#define VBR_SIZE_BOOT_SIG 1
#define VBR_OFFSET_VOL_ID 67
#define VBR_SIZE_VOL_ID 4
#define VBR_OFFSET_VOL_LAB 71
#define VBR_SIZE_VOL_LAB 11
#define VBR_OFFSET_FIL_SYS_TYPE 82
#define VBR_SIZE_FIL_SYS_TYPE 8
#define VBR_OFFSET_BOOT_CODE_32 90
#define VBR_SIZE_BOOT_CODE_32 420
#define VBR_OFFSET_BOOT_SIGN 510
#define VBR_SIZE_BOOT_SIGN 2
#define FSINFO_OFFSET_LEAD_SIG 0
#define FSINFO_SIZE_LEAD_SIG 4
#define FSINFO_OFFSET_RESERVED 4
#define FSINFO_SIZE_RESERVED 480
#define FSINFO_OFFSET_STRUCT_SIG 484
#define FSINFO_SIZE_STRUCT_SIG 4
#define FSINFO_OFFSET_FREE_COUNT 488
#define FSINFO_SIZE_FREE_COUNT 4
#define FSINFO_OFFSET_NXT_FREE 492
#define FSINFO_SIZE_NXT_FREE 4
#define FSINFO_OFFSET_RESERVED2 496
#define FSINFO_SIZE_RESERVED2 12
#define FSINFO_OFFSET_TRAIL_SIG 508
#define FSINFO_SIZE_TRAIL_SIG 4

/*size is used only to perform a check and is optional*/
template<class T>
void write_at(volatile char* base, uint32_t offset, T data, uint32_t size = 0){
	assert(size == 0 || size == sizeof(T));
	*(T*)(base+offset) = data;
}

/*size is used only to perform a check and is optional*/
template<class T>
T read_at(const volatile char* base, uint32_t offset, uint32_t size = 0){
	assert(size == 0 || size == sizeof(T));
	return *(T*)(base+offset);
}

//TODO: review
uint32_t getFSInfoSectorOffset(const char* vbr){
	return read_at<uint16_t>(vbr, VBR_OFFSET_FS_INFO, VBR_SIZE_FS_INFO);
}

uint32_t getFatAreaStart(const char* vbr){
	return read_at<uint16_t>(vbr, VBR_OFFSET_RSVD_SEC_CNT, VBR_SIZE_RSVD_SEC_CNT);
}

//TODO: review

//in sectors
uint32_t getFatAreaSize(const char* vbr){
	return read_at<uint8_t>(vbr, VBR_OFFSET_NUM_FATS, VBR_SIZE_NUM_FATS) * read_at<uint32_t>(vbr, VBR_OFFSET_FAT_SZ_32, VBR_SIZE_FAT_SZ_32);
}

//TODO: review

uint32_t getDataAreaFirstSector(const char* vbr){
	uint32_t result = read_at<uint16_t>(vbr, VBR_OFFSET_RSVD_SEC_CNT, VBR_SIZE_RSVD_SEC_CNT) +
	(read_at<uint8_t>(vbr, VBR_OFFSET_NUM_FATS, VBR_SIZE_NUM_FATS) * read_at<uint32_t>(vbr, VBR_OFFSET_FAT_SZ_32, VBR_SIZE_FAT_SZ_32));
	return result;
}

//TODO: review

uint32_t getDataAreaSize(const char* vbr){
	uint32_t result = read_at<uint32_t>(vbr, VBR_OFFSET_TOT_SEC_32, VBR_SIZE_TOT_SEC_32) - getDataAreaFirstSector(vbr);
	return result;
}

//TODO: review

uint32_t getClusterCount(const char* vbr){
	return getDataAreaSize(vbr) / read_at<uint8_t>(vbr, VBR_OFFSET_SEC_PER_CLUS, VBR_SIZE_SEC_PER_CLUS);
}

uint32_t getSectorSizeInBytes(const char* vbr){
	return read_at<uint16_t>(vbr, VBR_OFFSET_BYTS_PER_SEC, VBR_SIZE_BYTS_PER_SEC);
}

/*
 * vbr_in -> pointer to a buffer were the vbr will be written, it MUST be at LEAST the size of the sector 
 * volume_name -> name of the volume, the max size is 11 chars
 * size_of_fat_in_sectors -> no need to describe it 
 * size_of_volume_in_sectors -> total fat32 volume size in sector AKA the partition size
 * cluster_size_in_sectors -> the size of the fat32 allocation unit in sectors, it shouldn't be over the 32KB(cluster_size_in_sectors*sector_size) for compatibility reasons
 * sector_size -> you can change it but it'll probably break everything
 */
char* makeFat32VBR(char* vbr_in, std::string volume_name, uint32_t size_of_fat_in_sectors, uint32_t size_of_volume_in_sectors, uint8_t cluster_size_in_sectors, uint16_t sector_size = 512){
	const uint8_t number_of_fats = 2;
	const uint16_t reserved_sectors_count = 32;//typical value for fat32 drives
	if(reserved_sectors_count < 1){
		std::cerr << "[ERROR] the reserved sector must be at least 1(the VBR)\n";
		exit(-1);
	}
	if(cluster_size_in_sectors*sector_size > 32*KB){
		std::cerr << "[WARNING] the cluster size is over 32KB, it can cause compatibility issues\n";
	}
	if(volume_name.size() > VBR_SIZE_VOL_LAB){
		std::cerr << "[ERROR] the volume label cannot bigger than 11 chars\n";
		exit(-1);
	}

	memset(vbr_in, 0, sector_size);

	char name[]{'M','S','W','I','N','4','.','1'};
	memcpy(vbr_in + VBR_OFFSET_OEM_NAME, name, sizeof(name));

	write_at<uint8_t>(vbr_in, VBR_OFFSET_JMP_BOOT, 0xEB);//jump opcode
	write_at<uint8_t>(vbr_in+1, VBR_OFFSET_JMP_BOOT, (-2 + VBR_OFFSET_BOOT_CODE_32));//-2 because the next instruction will be after 2 bytes
	write_at<uint8_t>(vbr_in+2, VBR_OFFSET_JMP_BOOT, 0x90);//nop opcode
	
	write_at<uint16_t>(vbr_in, VBR_OFFSET_BYTS_PER_SEC, sector_size, VBR_SIZE_BYTS_PER_SEC);
	//assert(read_at<uint16_t>(vbr_in, VBR_OFFSET_BYTS_PER_SEC, VBR_SIZE_BYTS_PER_SEC) == sector_size);
	write_at<uint8_t>(vbr_in, VBR_OFFSET_SEC_PER_CLUS, cluster_size_in_sectors, VBR_SIZE_SEC_PER_CLUS);
	write_at<uint16_t>(vbr_in, VBR_OFFSET_RSVD_SEC_CNT, reserved_sectors_count, VBR_SIZE_RSVD_SEC_CNT);
	write_at<uint8_t>(vbr_in, VBR_OFFSET_NUM_FATS, number_of_fats, VBR_SIZE_NUM_FATS);//for compatibility problems it must be 2
	write_at<uint16_t>(vbr_in, VBR_OFFSET_ROOT_ENT_CNT, 0, VBR_SIZE_ROOT_ENT_CNT);//on fat32 it's 0
	write_at<uint16_t>(vbr_in, VBR_OFFSET_TOT_SEC_16, 0, VBR_SIZE_TOT_SEC_16);//on fat32 it's deprecated and must be 0
	write_at<uint8_t>(vbr_in, VBR_OFFSET_MEDIA, 0xF0, VBR_SIZE_MEDIA);//magic number
	write_at<uint16_t>(vbr_in, VBR_OFFSET_FAT_SZ_16, 0, VBR_SIZE_FAT_SZ_16);//on fat32 it's deprecated and must be 0
	write_at<uint32_t>(vbr_in, VBR_OFFSET_HIDD_SEC, 0, VBR_SIZE_HIDD_SEC);//used only on unpartitioned drives
	write_at<uint32_t>(vbr_in, VBR_OFFSET_TOT_SEC_32, size_of_volume_in_sectors, VBR_SIZE_TOT_SEC_32);
	
	write_at<uint32_t>(vbr_in, VBR_OFFSET_FAT_SZ_32, size_of_fat_in_sectors, VBR_SIZE_FAT_SZ_32);
	assert( read_at<uint32_t>(vbr_in, VBR_OFFSET_FAT_SZ_32, VBR_SIZE_FAT_SZ_32) == size_of_fat_in_sectors);
	
	write_at<uint16_t>(vbr_in, VBR_OFFSET_EXT_FLAGS, ACTIVE_FAT_FLAG(1), VBR_SIZE_EXT_FLAGS);//set the first fat active
	write_at<uint16_t>(vbr_in, VBR_OFFSET_FS_VER, 0, VBR_SIZE_FS_VER);
	write_at<uint32_t>(vbr_in, VBR_OFFSET_ROOT_CLUS, 2, VBR_SIZE_ROOT_CLUS);//decided by me
	write_at<uint16_t>(vbr_in, VBR_OFFSET_FS_INFO, 1, VBR_SIZE_FS_INFO);//decided by me
	write_at<uint16_t>(vbr_in, VBR_OFFSET_BK_BOOT_SEC, 6, VBR_SIZE_BK_BOOT_SEC);
	write_at<uint8_t>(vbr_in, VBR_OFFSET_DRV_NUM, 0x80, VBR_SIZE_DRV_NUM);
	write_at<uint8_t>(vbr_in, VBR_OFFSET_BS_RESERVED, 0, VBR_SIZE_BS_RESERVED);
	write_at<uint8_t>(vbr_in, VBR_OFFSET_BOOT_SIG, 0x29, VBR_SIZE_BOOT_SIG);//magic number
	write_at<uint32_t>(vbr_in, VBR_OFFSET_VOL_ID, (uint32_t)time(NULL), VBR_SIZE_VOL_ID);

	memset(vbr_in + VBR_OFFSET_VOL_LAB, ' ', VBR_SIZE_VOL_LAB);
	for(int i = 0; i < volume_name.size(); i++)
		(vbr_in+VBR_OFFSET_VOL_LAB)[i] = volume_name[i];

	memcpy(vbr_in + VBR_OFFSET_FIL_SYS_TYPE, "FAT32   ", VBR_SIZE_FIL_SYS_TYPE);
	write_at<uint16_t>(vbr_in, VBR_OFFSET_BOOT_SIGN, 0x55AA, VBR_SIZE_BOOT_SIGN);

	uint32_t total_size = read_at<uint16_t>(vbr_in, VBR_OFFSET_RSVD_SEC_CNT, VBR_SIZE_RSVD_SEC_CNT) + (number_of_fats*size_of_fat_in_sectors) -1;
	
	std::cerr << "[INFO]reserved sectors size: " << read_at<uint16_t>(vbr_in, VBR_OFFSET_RSVD_SEC_CNT, VBR_SIZE_RSVD_SEC_CNT) << std::endl;
	std::cerr << "[INFO]FAT sectors size: " <<  getFatAreaSize(vbr_in) << std::endl;
	std::cerr << "[INFO]Data sectors size: " << getDataAreaSize(vbr_in) << std::endl;
	assert(getDataAreaSize(vbr_in) + getFatAreaSize(vbr_in) + read_at<uint16_t>(vbr_in, VBR_OFFSET_RSVD_SEC_CNT, VBR_SIZE_RSVD_SEC_CNT) == size_of_volume_in_sectors);

	
	if(total_size > size_of_volume_in_sectors){
		std::cerr << "[ERROR] the size of the volume is too small for the used parameters\n";
		exit(-1);
	}
	
	if(getClusterCount(vbr_in) < MIN_FAT32_CLUSTERS){
		std::cerr << "[ERROR] there aren't enough clusters for the volume to be a fat32(clusters="<< getClusterCount(vbr_in) << ") there should be at least " << MIN_FAT32_CLUSTERS << "\n";
		exit(-1);
	}
	return vbr_in;
}

void writeBootCode(char* vbr, std::string boot_code_path){
	char code[VBR_SIZE_BOOT_CODE_32];
	std::ifstream boot_code_file(boot_code_path, std::ios::binary | std::ios::in);
	//get size of the file
	boot_code_file.seekg( 0, std::ios::end );
	uint32_t size = boot_code_file.tellg();
	assert(size == VBR_SIZE_BOOT_CODE_32);

	boot_code_file.seekg(0);
	boot_code_file.read(code, VBR_SIZE_BOOT_CODE_32);
	memcpy(vbr+VBR_OFFSET_BOOT_CODE_32, code, VBR_SIZE_BOOT_CODE_32);
	std::cerr << "[INFO] copied second stage code\n";
}

void makeFSInfoSector(char* fs_info_in, const char* vbr){
	uint32_t sector_size = getSectorSizeInBytes(vbr);
	memset(fs_info_in, 0, sector_size);

	uint32_t root_cluster = read_at<uint32_t>(vbr, VBR_OFFSET_ROOT_CLUS, VBR_SIZE_ROOT_CLUS);
	
	//magic number
	write_at<uint32_t>(fs_info_in, FSINFO_OFFSET_LEAD_SIG, 0x41615252, FSINFO_SIZE_LEAD_SIG);

	//magic number
	write_at<uint32_t>(fs_info_in, FSINFO_OFFSET_STRUCT_SIG, 0x61417272, FSINFO_SIZE_STRUCT_SIG);

	write_at<uint32_t>(fs_info_in, FSINFO_OFFSET_FREE_COUNT, getClusterCount(vbr), FSINFO_SIZE_FREE_COUNT);
	write_at<uint32_t>(fs_info_in, FSINFO_OFFSET_NXT_FREE, root_cluster+1, FSINFO_SIZE_NXT_FREE);

	//magic number
	write_at<uint32_t>(fs_info_in, FSINFO_OFFSET_TRAIL_SIG, 0xAA550000, FSINFO_SIZE_TRAIL_SIG);
}

class FileImage{
public:
	FileImage(std::string path, int volume_size, int sector_size, int file_offset)
		: sector_size(sector_size), file_offset(file_offset){
		file.open(path, std::ios::binary | std::ios::out | std::ios::in);
		if(!file.is_open()){
			std::cerr << "error cannot open " << path << std::endl;
			exit(-1);
		}
	}
	void writeSector(int sector_number, char* sector_data){
		file.seekp(sector_number*sector_size + file_offset*sector_size);
		file.write(sector_data, sector_size);
	}
	void readSector(int sector_number, char* sector_data){
		file.seekg(sector_number*sector_size + file_offset*sector_size);
		file.read(sector_data, sector_size);
	}
	int getSectorSize(){
		return sector_size;
	}
private:
	int sector_size;
	int file_offset;
	std::fstream file;
};

uint32_t readFatEntry(FileImage& image, uint32_t entry){
	auto sector_buffer = std::make_unique<char[]>(image.getSectorSize());
	image.readSector(0, sector_buffer.get());
	uint32_t sector = getFatAreaStart(sector_buffer.get()) + (entry*4 / image.getSectorSize());
	image.readSector(sector, sector_buffer.get());
	return read_at<uint32_t>(sector_buffer.get(), (entry*4)% image.getSectorSize());
}

void writeFatEntry(FileImage& image, uint32_t entry, uint32_t value){
	auto sector_buffer = std::make_unique<char[]>(image.getSectorSize());
	image.readSector(0, sector_buffer.get());
	uint32_t sector = getFatAreaStart(sector_buffer.get()) + (entry*4 / image.getSectorSize());

	image.readSector(sector, sector_buffer.get());
	write_at<uint32_t>(sector_buffer.get(), (entry*4)% image.getSectorSize(), value);
	image.writeSector(sector, sector_buffer.get());
}

uint32_t getFirstSectorOfCluster(const char* vbr, uint32_t cluster){
	uint32_t sector = getDataAreaFirstSector(vbr) + (cluster-2)*getSectorSizeInBytes(vbr);
	return sector;
}

uint32_t getClusterSizeInSectors(const char* vbr){
	return read_at<uint8_t>(vbr, VBR_OFFSET_SEC_PER_CLUS, VBR_SIZE_SEC_PER_CLUS);
}

void initializeFileAllocationTable(FileImage& image){
	auto sector_buffer = std::make_unique<char[]>(image.getSectorSize());
	image.readSector(0, sector_buffer.get());
	int size = getFatAreaSize(sector_buffer.get());
	int start = getFatAreaStart(sector_buffer.get());

	//zero out the fat
	memset(sector_buffer.get(), 0, image.getSectorSize());
	for(int i = 0; i < size; i++){
		image.writeSector(start+i, sector_buffer.get());
	}

	//set FAT[0] and FAT[1]
	image.readSector(0, sector_buffer.get());
	writeFatEntry(image, 0, 0xFFFFFF00 | read_at<uint8_t>(sector_buffer.get(), VBR_OFFSET_MEDIA, VBR_SIZE_MEDIA));
	writeFatEntry(image, 1, 0xFFFFFFFF);
}

//TODO: review
void updateFat32Backup(FileImage& image){
	auto sector_buffer = std::make_unique<char[]>(image.getSectorSize());
	image.readSector(0, sector_buffer.get());
	uint32_t backup_sector_start = read_at<uint16_t>(sector_buffer.get(), VBR_OFFSET_BK_BOOT_SEC, VBR_SIZE_BK_BOOT_SEC);

	const uint32_t backup_size_in_sectors = 3;//sectors from 0 to 2 inclusive
	for(int i = 0; i < backup_size_in_sectors; i++){
		image.readSector(i, sector_buffer.get());
		image.writeSector(i+backup_sector_start, sector_buffer.get());
	}
	return;
}

void makeRootDirectory(FileImage& image){
	auto sec_buffer = std::make_unique<char[]>(image.getSectorSize());
	auto vbr_buffer = std::make_unique<char[]>(image.getSectorSize());
	image.readSector(0, vbr_buffer.get());

	//set the fat entry as the end of chain
	uint32_t first_sector_of_cluster = getFirstSectorOfCluster(vbr_buffer.get(), 2);
	uint32_t cluster_size = getClusterSizeInSectors(vbr_buffer.get());

	//set every directory entry in the cluster as free
	for(int s_off = 0; s_off < cluster_size; s_off++){
		image.readSector(first_sector_of_cluster + s_off, sec_buffer.get());
		for(int b_off = 0; b_off < image.getSectorSize(); b_off+=32){
			write_at<uint8_t>(sec_buffer.get(), b_off, DIRECTORY_ENTRY_FREE_AND_AFTER_FREE);
		}
		image.writeSector(first_sector_of_cluster + s_off, sec_buffer.get());
	}
	writeFatEntry(image, read_at<uint32_t>(vbr_buffer.get(), VBR_OFFSET_ROOT_CLUS, VBR_SIZE_ROOT_CLUS), FAT_ENTRY_END_OF_CHAIN);
}

std::pair<int, bool> tryParseIntOption(std::string opt){
	for(auto c : opt)
		if(!std::isdigit(c))
			return {0, false};

	std::stringstream ss;
	ss << opt;
	int out;
	ss >> out;
	return {out, true};
}

bool containsPrefix(std::string prefix, std::string str){
	return prefix == str.substr(0, prefix.size());
}

void readIntParamIfPresent(int* param, std::string arg, std::string prefix){
		if(containsPrefix(prefix, arg)){
			auto parsed = tryParseIntOption(arg.substr(prefix.size(), arg.size()-prefix.size()));
			if(!parsed.second){
				std::cerr << "[ERROR] " << prefix << " the parameter isn't a number\n" << std::endl;
			}
			*param = parsed.first;
		}
}

void readStringParamIfPresent(std::string* out_value, std::string arg, std::string prefix){
		if(containsPrefix(prefix, arg)){
			*out_value = arg.substr(prefix.size(), arg.size()-prefix.size());
		}
}

int main(int argc, char** argv){
	std::vector<std::string> args;
	for(int i = 1; i < argc; i++)
		args.emplace_back(argv[i]);

	int fat_size = 128, volume_size = -1, cluster_size = 1, sector_size = 512, file_offset = 0;
	std::string file_path="";
	std::string boot_code_path="";

	for(auto arg : args){
		readIntParamIfPresent(&fat_size, arg, "fat_size=");
		readIntParamIfPresent(&volume_size, arg, "volume_size=");
		readIntParamIfPresent(&cluster_size, arg, "cluster_size=");
		readIntParamIfPresent(&sector_size, arg, "sector_size=");
		readIntParamIfPresent(&file_offset, arg, "file_offset=");
		readStringParamIfPresent(&file_path, arg, "file=");
		readStringParamIfPresent(&boot_code_path, arg, "boot_code=");
	}

	if(file_path.size() == 0){
		std::cerr << "file=<file path> isn't present as parameter\n";
		exit(-1);
	}

	if(volume_size < 0){
		std::cerr << "volume_size=<size in sectors> isn't present as parameter\n";
		exit(-1);
	}

	FileImage image(file_path, volume_size, sector_size, file_offset);

	char* vbr = (char*)malloc(sector_size);
	char* fs_info = (char*)malloc(sector_size);

	makeFat32VBR(vbr, "test", fat_size, volume_size, cluster_size, sector_size);
	if(boot_code_path.size() != 0){
		std::cerr << "[INFO] going to copy second stage code\n";
		writeBootCode(vbr, boot_code_path);
	}
	image.writeSector(0, vbr);
	makeFSInfoSector(fs_info, vbr);
	image.writeSector(getFSInfoSectorOffset(vbr), fs_info);
	updateFat32Backup(image);
	initializeFileAllocationTable(image);
	makeRootDirectory(image);

	free(vbr);
	free(fs_info);
	return 0;
}
