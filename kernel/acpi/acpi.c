#include "acpi.h"
#include "../kio.h"

bool check_rsdp(RSDP* rsdp){
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

bool check_xsdp(XSDP* xsdp){
	uint64_t* singature_ptr = (uint64_t*)"RSD PTR ";
	if(xsdp->signature != *singature_ptr)
		return false;

	uint8_t* raw_xsdp = (uint8_t*)xsdp;
	uint8_t checksum = 0;
	for(size_t i = 0; i < xsdp->len; i++)
		checksum += raw_xsdp[i];

	return checksum == 0;
}

bool parse_acpi_rsdt(RSDP* rsdp){
	if(!check_rsdp(rsdp))
		return false;
	return true;
}

bool parse_acpi_xsdt(XSDP* xsdp){
	if(!check_rsdp((RSDP*)xsdp))
		return false;

	if(!check_xsdp(xsdp))
		return false;
	return false;
}

bool acpi_init(){
	RSDP rsdp;
	XSDP xsdp;

	void* rsdp_ptr = acpi_RSDP();
	//the rounded up division of the size of XSDP struct and the page size
	uint64_t number_of_pages = 1 + (sizeof(XSDP)/PAGE_SIZE + (sizeof(XSDP)%PAGE_SIZE != 0));
	RSDP* original_rsdp = rsdp_ptr;
	
	bool is_acpi2 = original_rsdp->revision == ACPI_REVISION2;
	if(is_acpi2)
		xsdp = *(XSDP*)original_rsdp;//copy in a local struct 
	rsdp = *original_rsdp;

	if(is_acpi2){
		if(!parse_acpi_xsdt(&xsdp))
			return false;
	}else{
		if(!parse_acpi_rsdt(&rsdp))
			return false;
	}

	

	return true;
}
