#pragma once
#include "acpi.h"
//for more info on the data structure see the ACPI specification

//ICS = Interrupt Controller Structure
#define ICS_TYPE_PROCESSOR_LOCAL_APIC 0
#define ICS_TYPE_PROCESSOR_IO_APIC 1
#define ICS_TYPE_INTERRUPT_SOURCE_OVERRIDE 2
#define ICS_TYPE_NMI_SOURCE 3
#define ICS_TYPE_LOCAL_APIC_NMI 4
#define ICS_TYPE_LOCAL_APIC_ADDRESS_OVERRIDE 5
#define ICS_TYPE_IO_SAPIC 6
#define ICS_TYPE_LOCAL_SAPIC 7
#define ICS_TYPE_PLATFORM_INTERRUPT_SOURCES 8
#define ICS_TYPE_PROCESSOR_LOCAL_X2APIC 9
#define ICS_TYPE_LOCAL_X2APIC_NMI 10
#define ICS_TYPE_GIC_CPU_INTERFACE 11
#define ICS_TYPE_GIC_DISTRIBUTOR 12
#define ICS_TYPE_GIC_MSI_FRAME 13
#define ICS_TYPE_GIC_REDISTRIBUTOR 14
#define ICS_TYPE_GIC_ITS 15
#define ICS_TYPE_MULTIPROCESSO_WAKEUP 16
#define ICS_TYPE_CORE_PIC 17
#define ICS_TYPE_LIO_PIC 18
#define ICS_TYPE_HT_PIC 19
#define ICS_TYPE_EIO_PIC 20
#define ICS_TYPE_MSI_PIC 21
#define ICS_TYPE_BIO_PIC 22
#define ICS_TYPE_LPC_PIC 23

#define LOCAL_APIC_FLAG_ENABLED 1
#define LOCAL_APIC_FLAG_ONLINE_CAPABLE 1<<1

#define MPS_INTI_POLARITY_BUS_CONFORM 0
#define MPS_INTI_POLARITY_ACTIVE_HIGH 1
#define MPS_INTI_POLARITY_ACTIVE_LOW  3

#define MPS_INTI_TRIGGER_MODE_BUS_CONFORM 0 << 2
#define MPS_INTI_TRIGGER_MODE_EDGE_TRIGGERED 1 << 2
#define MPS_INTI_TRIGGER_MODE_LEVEL_TRIGGERED 3 << 2


typedef struct MADT{
	ACPI_description_header header;
	uint32_t local_interrupt_controller_address;
	uint32_t flags;
} __attribute__((packed)) MADT;

//ICS = Interrupt Controller Structure
typedef struct ICS_header{
	uint8_t type;
	uint8_t lenght;
} __attribute((packed)) ICS_header;

typedef struct ICS_local_APIC{
	ICS_header header;
	uint8_t ACPI_processor_UID;
	uint8_t APIC_ID;
	uint32_t flags;
} __attribute((packed)) ICS_local_APIC;

typedef struct ICS_IO_APIC{
	ICS_header header;
	uint8_t IO_APIC_ID;
	uint8_t reserved;
	uint32_t IO_APIC_address;
	uint32_t global_system_interrupt_base;
} __attribute((packed)) ICS_IO_APIC;

typedef struct ICS_interrupt_source_override{
	ICS_header header;
	uint8_t bus;
	uint8_t source;
	uint32_t global_system_interrupt;
	uint16_t flags;
} __attribute((packed)) ICS_interrupt_source_override;

typedef struct ICS_NMI{
	ICS_header header;
	uint16_t flags;
	uint32_t global_system_interrupt;
} __attribute((packed)) ICS_NMI;

typedef struct ICS_local_APIC_NMI{
	ICS_header header;
	uint8_t ACPI_processor_UID;
	uint16_t flags;
	uint8_t local_APIC_LINT;
} __attribute((packed)) ICS_local_APIC_NMI;

typedef struct ICS_local_APIC_address_override{
	ICS_header header;
	uint16_t reserved;
	uint64_t local_APIC_address;
} __attribute((packed)) ICS_local_APIC_address_override;

typedef struct ICS_processor_local_X2APIC{
	ICS_header header;
	uint16_t reserved;
	uint32_t X2APIC_ID;
	uint32_t flags;
	uint32_t ACPI_processor_UID;
} __attribute((packed)) ICS_processor_local_X2APIC;

typedef struct ICS_local_X2APIC_NMI{
	ICS_header header;
	uint16_t flags;
	uint32_t ACPI_processor_UID;
	uint8_t local_APIC_LINT;
	uint8_t reserved[3];
} __attribute((packed)) ICS_local_X2APIC_NMI;

//may return NULL if there is no MADT table or if the checksum is wrong
MADT* get_MADT();
ICS_header* get_first_ICS_header(const MADT* madt);
//return NULL when there is no next header
ICS_header* get_next_ICS_header(const MADT* madt, const ICS_header* header);
