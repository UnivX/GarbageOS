#pragma once
#include "../util/sync_types.h"
#include "../mem/vmm.h"
#include "../timer/pit.h"

typedef struct APKernelData{
	VMemHandle kernel_stack;
	uint32_t apic_id;
} APKernelData;

typedef enum MPInitError{
	ERROR_MP_OK = 0,
	ERROR_MP_PARTIAL_STARTUP = 1,
	ERROR_MP_STARTUP_FAILED = 2
} MPInitError;

MPInitError init_APs(PIT* pit);
