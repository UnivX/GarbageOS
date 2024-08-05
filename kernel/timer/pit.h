#pragma once
#include "../kdefs.h"
#include "../hal.h"
#define PIT_IRQ 0
#define PIT_FREQ_MHZ 1.193182

//read/write
#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_CHANNEL_1_DATA_PORT 0x41
#define PIT_CHANNEL_2_DATA_PORT 0x42
//write only
#define PIT_MODE_COMMAND_PORT 0x42

typedef struct PIT{
	//TODO make it atomic
	uint64_t tic_counter;//each tic is 1ms
	uint8_t interrupt_vector;
	bool channel_0_initialized;
	//channel 1 shall not be used
	bool channel_2_initialized;//used for pc speaker
} PIT;

PIT create_PIT(uint8_t interrupt_vector);
void initialize_PIT_timer(PIT* pit);
//tic is in ms, interrupts are needed for a 
uint64_t get_PIT_tic_count(PIT* pit);
void PIT_wait_ms(PIT* pit, uint32_t ms);
