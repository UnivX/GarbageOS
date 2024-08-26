#include "kdefs.h"
#include "hal.h"
#include "kio.h"
#include "timer/pit.h"
#include "mem/vmm.h"
#include "mem/heap.h"
#include "util/sync_types.h"

//TODO: make it more eficient or create a cache, check if we need sync or is read only
typedef struct LocalKernelData{
	VMemHandle kernel_stack;
	uint32_t apic_id;
} LocalKernelData;

//return the cpu_id
void init_kernel_data();
CPUID register_local_kernel_data_cpu();
void set_local_kernel_data(CPUID cpu_id, LocalKernelData data);
LocalKernelData get_local_kernel_data(CPUID cpu_id);
CPUID get_cpu_id_from_apic_id(uint32_t apic_id);

void init_kernel_data_PIT(uint64_t target_frequency_hz);
PIT* get_PIT_from_kernel_data();
