#pragma once
#include "../mem/vmm.h"
#include "../hal.h"
#include "../kdefs.h"

#define GAS_SYSTEM_MEMORY_SPACE 0x00
#define GAS_SYSTEM_IO_SPACE 0x01
#define GAS_PCI_CONFIGURATION_SPACE 0x02
#define GAS_EMBEDDED_CONTROLLER 0x03
#define GAS_SMBUS 0x04
#define GAS_SYSTEM_CMOS 0x05
#define GAS_PCI_BAR_TARGET 0x06
#define GAS_IPMI 0x07
#define GAS_GENERAL_PURPOSE_IO 0x08
#define GAS_GENERIC_SERIAL_BUS 0x09
#define GAS_PLATFORM_COMUNICATION_CHANNEL 0x0A
#define GAS_FUNCTIONAL_FIXED_HARDWARE 0x7F

#define GAS_BYTE_ACCESS 1
#define GAS_WORD_ACCESS 2
#define GAS_DWORD_ACCESS 3
#define GAS_QWORD_ACCESS 4

#define ACPI_REVISION2 2
#define MAX_RSDP_SIZE 64*KB

//ACPI Generic Address Structure
typedef struct GAS{
	uint8_t address_space_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t access_size;
	uint64_t address;
}__attribute__((packed)) GAS;

typedef struct RSDP {
 uint64_t signature;
 uint8_t checksum;
 char OEMID[6];
 uint8_t revision;
 uint32_t rsdt;
} __attribute__((packed)) RSDP;

typedef struct XSDP {
 uint64_t signature;
 uint8_t checksum;
 char OEMID[6];
 uint8_t revision;
 uint32_t rsdt;
 uint32_t len;
 uint64_t xsdt;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} __attribute__((packed)) XSDP;

bool acpi_init();
