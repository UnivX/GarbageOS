#pragma once
#include "../kdefs.h"
#include "../hal.h"
#define PIT_IRQ 0
#define PIT_FREQ_MHZ 1.193182
#define PIT_FREQ_HZ  1193182

//read/write
#define PIT_CHANNEL_0_DATA_PORT 0x40
#define PIT_CHANNEL_1_DATA_PORT 0x41
#define PIT_CHANNEL_2_DATA_PORT 0x42
//write only
#define PIT_MODE_COMMAND_PORT 0x42

//TODO: make multi processor sleep
typedef struct PIT{
	//TODO make it atomic
	uint64_t tick_counter;
	uint64_t interrupt_frequency;//Hz
	uint8_t interrupt_vector;
	bool channel_0_initialized;
	//channel 1 shall not be used
	bool channel_2_initialized;//used for pc speaker
} PIT;

PIT create_PIT(uint8_t interrupt_vector);

//set the pit with the target frequency, retuns the real frequency. The freqs are in Hz
uint64_t initialize_PIT_timer(PIT* pit, uint64_t target_frequency_hz);
//tick is in ms, interrupts are needed for a 
uint64_t get_PIT_tick_count(PIT* pit);
uint64_t get_ms_from_tick_count(PIT* pit, uint64_t tick);
void PIT_wait_ms(PIT* pit, uint64_t ms);
void PIT_wait_us(PIT* pit, uint64_t us);
