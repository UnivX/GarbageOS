#include "tss.h"
#include "../../mem/frame_allocator.h"


void init_tss(TSS* tss){
	memset(tss, 0, sizeof(TSS));
}
