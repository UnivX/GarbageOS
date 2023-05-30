#include "vbe.h"
volatile vbe_mode_info_structure* getVBE(){
	return *(volatile vbe_mode_info_structure**)(0x0600);
}
