#pragma once
#include "../acpi/madt.h"
#include "../mem/heap.h"
#include "../mem/vmm.h"
#define CPUID_APIC_PRESENCE_EDX 1<<9
#define IA32_APIC_BASE_MSR 0x1B
#define APIC_GLOBAL_ENABLE_DISABLE_FLAG 1 << 11

#define APIC_REG_OFFSET_LAPIC_ID 0x20
#define APIC_REG_OFFSET_LAPIC_VERSION 0x30
#define APIC_REG_OFFSET_TASK_PRIORITY 0x80
#define APIC_REG_OFFSET_ARBITRATION_PRIORITY 0x90
#define APIC_REG_OFFSET_PROCESSOR_PRIORITY 0xA0
#define APIC_REG_OFFSET_EOI 0xB0
#define APIC_REG_OFFSET_REMOTE_READ 0xC0
#define APIC_REG_OFFSET_LOCAL_DESTINATION 0xD0
#define APIC_REG_OFFSET_DESTINATION_FORMAT 0xE0
#define APIC_REG_OFFSET_SPURIOUS_INTERRUPT_VECTOR 0xF0
#define APIC_REG_OFFSET_IN_SERVICE_FIRST 0x100
#define APIC_REG_OFFSET_TRIGGER_MODE_FIRST 0x180
#define APIC_REG_OFFSET_INTERRUPT_REQUEST_FIRST 0x200
#define APIC_REG_OFFSET_ERROR_STATUS 0x280
#define APIC_REG_OFFSET_LVT_CMCI 0x2F0
#define APIC_REG_OFFSET_INTERRUPT_COMMAND_LOW 0x300
#define APIC_REG_OFFSET_INTERRUPT_COMMAND_HIGH 0x310
#define APIC_REG_OFFSET_LVT_TIMER 0x320
#define APIC_REG_OFFSET_LVT_THERMAL_SENSOR 0x330
#define APIC_REG_OFFSET_LVT_PERFORMANCE_MONITORING_COUNTERS 0x340
#define APIC_REG_OFFSET_LVT_LINT0 0x350
#define APIC_REG_OFFSET_LVT_LINT1 0x360
#define APIC_REG_OFFSET_LVT_ERROR 0x370
#define APIC_REG_OFFSET_INITIAL_COUNT 0x380
#define APIC_REG_OFFSET_CURRENT_COUNT 0x390
#define APIC_REG_OFFSET_DIVIDE_CONFIGURATION 0x3E0

#define APIC_REGISTER_SPACE_SIZE 4096
#define SUPPRESS_EOI_BROADCAST_VERSION_FLAG 1 << 23

typedef enum APICDeliveryMode{
	APIC_DEL_MODE_FIXED = 0b000,
	APIC_DEL_MODE_LOWEST_PRIORITY = 0b001,
	APIC_DEL_MODE_SMI = 0b010,
	APIC_DEL_MODE_NMI = 0b100,
	APIC_DEL_MODE_INIT = 0b101,
	APIC_DEL_MODE_START_UP = 0b110,
	APIC_DEL_MODE_EXT_INT = 0b111
} APICDeliveryMode;

typedef enum APICDestinationShorthand{
	APIC_DESTSH_NO_SH = 0b00,
	APIC_DESTSH_SELF = 0b01,
	APIC_DESTSH_ALL_INCLUDING_SELF = 0b10,
	APIC_DESTSH_ALL_EXCLUDING_SELF = 0b11
} APICDestinationShorthand;

//do not support X2APIC

typedef struct LocalAPIC{
	uint32_t APIC_ID;
	ICS_local_APIC* ics_lapic;
	ICS_local_APIC_NMI* ics_lapic_nmi;//may be null
	bool enabled;
	uint64_t apic_timer_counter;
	uint64_t frequency;
} LocalAPIC;

typedef struct LAPICSubsystemData{
	LocalAPIC* lapic_array;
	uint64_t lapic_array_size;
	void* lapic_base_address;
	VMemHandle register_space_mapping;
} LAPICSubsystemData;

bool init_apic();
bool is_local_apic_initialized();
uint32_t get_logical_core_lapic_id();
void print_lapic_state();

//check the in service register(ISR) to check if the cpu is servicing a fixed interrupt coming from the lapic
bool is_interrupt_lapic_generated(uint8_t interrupt_vector);
//end of interrupt
void send_lapic_EOI();

void send_IPI_by_destination_shorthand(APICDestinationShorthand dest_sh, uint8_t interrupt_vector);
void send_IPI_by_lapic_id(uint32_t lapic_id_target, uint8_t interrupt_vector);
void send_IPI_INIT_by_lapic_id(uint32_t lapic_id_target);
void send_IPI_INIT_to_all_excluding_self();
void send_IPI_INIT_deassert();//send to all logical cores
bool is_IPI_sending_complete();
