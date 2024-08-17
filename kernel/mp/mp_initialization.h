#pragma once
#include "../util/sync_types.h"
#include "../mem/vmm.h"
#include "../timer/pit.h"

typedef struct APKernelData{
	VMemHandle kernel_stack;
	uint32_t apic_id;
} APKernelData;

void init_APs(PIT* pit);
