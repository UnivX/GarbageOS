#include "mp_init_bin.h"
#include "../../kdefs.h"
#include "../../hal.h"

extern const uint8_t _binary_obj_HAL_bios_mp_init_o_assembled_start[];
extern const uint8_t _binary_obj_HAL_bios_mp_init_o_assembled_end[];
const uint8_t* mp_init_bin_start = _binary_obj_HAL_bios_mp_init_o_assembled_start;
const uint8_t* mp_init_bin_end = _binary_obj_HAL_bios_mp_init_o_assembled_end;

volatile void* create_mp_init_routine_in_RAM(const void* paging_structure, MPInitializationData* init_data){
	volatile void* startup_frame = get_real_mode_startup_frame();
	volatile void* cr3_copy = get_real_mode_secondary_startup_frame();

	//copy cr3 page
	for(uint64_t i = 0; i < PAGE_SIZE; i++)
		((uint8_t*)cr3_copy)[i] = ((uint8_t*)paging_structure)[i];

	KASSERT(mp_init_bin_end - mp_init_bin_start == PAGE_SIZE);
	for(uint64_t i = 0; i < PAGE_SIZE; i++)
		((uint8_t*)startup_frame)[i] = mp_init_bin_start[i];

	*(uint64_t*)(startup_frame+PAGE_SIZE-16) = (uint64_t)cr3_copy;
	*(uint64_t*)(startup_frame+PAGE_SIZE-8) = (uint64_t)init_data;


	return startup_frame;
}
