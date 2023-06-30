#include "tss.h"
#include "../../frame_allocator.h"


void init_tss(TSS* tss){
	memset(tss, 0, sizeof(TSS));
}
