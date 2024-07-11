#include "acpi.h"
#include "../kio.h"

static RSDP* cached_rsdp = NULL;
static XSDP* cached_xsdp = NULL;

bool check_rsdp(const RSDP* rsdp){
	char string_signature[] = "RSD PTR ";
	uint64_t signature = *(uint64_t*)string_signature;

	if(rsdp->signature != signature){
		return false;
	}

	uint8_t* raw_rsdp = (uint8_t*)rsdp;
	uint8_t checksum = 0;
	for(size_t i = 0; i < sizeof(RSDP); i++)
		checksum += raw_rsdp[i];

	return checksum == 0;
}

bool check_xsdp(const XSDP* xsdp){
	return check_rsdp((RSDP*)xsdp);
}

bool check_ACPI_table(const ACPI_description_header* header){
	uint8_t* raw_table = (uint8_t*)header;
	uint8_t checksum = 0;
	for(size_t i = 0; i < header->lenght; i++)
		checksum += raw_table[i];

	return checksum == 0;
}

ACPI_description_header* get_rsdt(const RSDP* rsdp){
	uint64_t addr = rsdp->rsdt;
	return (ACPI_description_header*)addr;
};

ACPI_description_header* get_xsdt(const XSDP* xsdp){
	uint64_t addr = xsdp->xsdt;
	return (ACPI_description_header*)addr;
};

uint64_t get_acpi_table_headerless_lenght(const ACPI_description_header* table){
	return table->lenght - sizeof(ACPI_description_header);
}

bool print_acpi_rsdt(){
	KASSERT(cached_rsdp != NULL);
	ACPI_description_header* rsdt = get_rsdt(cached_rsdp);
	uint32_t* entries = ((void*)rsdt + sizeof(ACPI_description_header));
	uint64_t number_of_entries = get_acpi_table_headerless_lenght(rsdt) / sizeof(uint32_t);

	char print_buffer[6] = "****\n\0";

	print("ACPI table signatures: \n\n");
	for(uint64_t i = 0; i < number_of_entries; i++){
		ACPI_description_header* header = (ACPI_description_header*)((uint64_t)entries[i]);
		*(uint32_t*)print_buffer = header->signature;
		print(print_buffer);
	}

	return true;
}

ACPI_description_header* get_table_header(uint32_t signature){
	KASSERT(cached_rsdp != NULL || cached_xsdp != NULL);
	//if cached_xsdp is NULL then the ACPI version is 1 and there only the rsdp
	//in the ACPI 1 version the table entries are 4 byte addresses. In the ACPI 2 version are 8 byte addresses
	uint64_t number_of_entries = 0;
	void* entries = NULL;
	if(cached_xsdp != NULL){
		ACPI_description_header* xsdt = get_xsdt(cached_xsdp);
		entries = (void*)xsdt + sizeof(ACPI_description_header);
		number_of_entries = get_acpi_table_headerless_lenght(xsdt) / sizeof(uint64_t);
	}else{
		ACPI_description_header* rsdt = get_rsdt(cached_rsdp);
		entries = (void*)rsdt + sizeof(ACPI_description_header);
		number_of_entries = get_acpi_table_headerless_lenght(rsdt) / sizeof(uint32_t);
	}
	
	for(uint64_t i = 0; i < number_of_entries; i++){
		ACPI_description_header* header;
		if(cached_xsdp == NULL){
			//if it's rsdt
			header = (ACPI_description_header*)( (uint64_t) ((uint32_t*)entries)[i] );
		}else{
			//if it's xsdt
			header = (ACPI_description_header*) ((uint64_t*)entries)[i];
		}
		
		if(header->signature == signature)
			return header;
	}
	return NULL;
}

bool acpi_init(){
	cached_rsdp = (RSDP*) acpi_RSDP();
	
	bool is_acpi2 = cached_rsdp->revision == ACPI_REVISION2;
	if(is_acpi2)
		cached_xsdp = (XSDP*)cached_rsdp;//copy in a local struct 

	if(is_acpi2){
		if(!check_xsdp(cached_xsdp)){
			cached_xsdp = NULL;
			cached_rsdp = NULL;
			return false;
		}
	}else{
		if(!check_rsdp(cached_rsdp)){
			cached_xsdp = NULL;
			cached_rsdp = NULL;
			return false;
		}
	}
	return true;
}
