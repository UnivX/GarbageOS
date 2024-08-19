#include "pit.h"
#include "../interrupt/interrupts.h"
#include "../interrupt/ioapic.h"
#include "../interrupt/apic.h"

static void PIT_interrupt_handler(InterruptInfo info);

PIT create_PIT(uint8_t interrupt_vector){
	PIT pit;
	pit.channel_0_initialized = false;
	pit.channel_2_initialized = false;
	pit.interrupt_vector = interrupt_vector;
	pit.tick_counter = 0;
	return pit;
}

void enable_PIT_irq(PIT* pit){
	//enable the irq in the IOApic
	IOAPICInterruptRedirection int_redi;
	int_redi.vector = pit->interrupt_vector;
	int_redi.mask = false;

	//send interrupt to lowest priority processor
	//int_redi.apic_id_to_redirect = 0;
	//int_redi.reidirect_to_lowest_priority = true;

	int_redi.apic_id_to_redirect = get_logical_core_lapic_id();
	int_redi.reidirect_to_lowest_priority = false;

	//set polarity and trigger mode to the default values
	//if the MADT specify the polarity and the trigger mode
	//the IOAPIC code will take care of it
	int_redi.polarity = 0; //high active
	int_redi.trigger_mode = 0;//edge_sensitive

	setIOAPICInterruptRedirection(int_redi, PIT_IRQ);
}

//LSB = least significant bit, MSB most significant bit
typedef enum ReadWriteMode{
	READ_WRITE_COUNTER_LATCH = 0,
	READ_WRITE_LSB_ONLY = 1,
	READ_WRITE_MSB_ONLY = 2,
	READ_WRITE_LSB_FIRST_MSB_THEN = 3
}ReadWriteMode;

//select_counter = put the channel number or 4 for read-back
//mode = 0-5 PIT mode
uint8_t make_control_word(uint8_t select_counter, ReadWriteMode read_write, uint8_t mode){
	KASSERT(mode <= 5);
	KASSERT(select_counter <= 4);
	uint8_t control_word = 0;
	//bcd = 0
	control_word = (mode & 0b00000111) << 1;
	control_word = (read_write & 0b00000011) << 4;
	control_word = (select_counter & 0b00000011) << 6;
	return control_word;
}

void set_mode0_on_channel0(PIT* pit, uint16_t counter_value){
	//TODO: check if the pit is already counting(sync primitive)
	uint8_t control_word = make_control_word(0, READ_WRITE_LSB_FIRST_MSB_THEN, 0);
	InterruptState istate = disable_and_save_interrupts();
	outb(PIT_MODE_COMMAND_PORT, control_word);
	outw(PIT_CHANNEL_0_DATA_PORT, counter_value);
	restore_interrupt_state(istate);
}

void set_mode2_on_channel0(PIT* pit, uint16_t counter_value){
	//TODO: check if the pit is already counting(sync primitive)
	uint8_t control_word = make_control_word(0, READ_WRITE_LSB_FIRST_MSB_THEN, 2);
	uint8_t counter_value_low = counter_value;
	uint8_t counter_value_high = counter_value >> 8;

	InterruptState istate = disable_and_save_interrupts();
	outb(PIT_MODE_COMMAND_PORT, control_word);
	outb(PIT_CHANNEL_0_DATA_PORT, counter_value_low);
	outb(PIT_CHANNEL_0_DATA_PORT, counter_value_high);
	restore_interrupt_state(istate);
}

//approximate the counter value from the frequency
static uint16_t get_counter_value_from_frequency(uint64_t target_frequency_hz){
	uint64_t counter_value = PIT_FREQ_HZ / target_frequency_hz;
	uint64_t remainder = PIT_FREQ_HZ % target_frequency_hz;
	if(remainder > PIT_FREQ_HZ/2)
		counter_value++;
	
	if(counter_value > 0xffff)
		counter_value = 0xffff;
	return (uint16_t)counter_value;
}

//get the exact frequency from the counter_value
static uint64_t get_frequency_hz_from_counter_value(uint16_t counter_value){
	uint64_t freq = PIT_FREQ_HZ / counter_value;
	uint64_t remainder = PIT_FREQ_HZ % counter_value;
	if(remainder > PIT_FREQ_HZ/2)
		freq++;
	return freq;
}

uint64_t initialize_PIT_timer(PIT* pit, uint64_t target_frequency_hz){
	//set interrupt handler
	install_interrupt_handler(pit->interrupt_vector, PIT_interrupt_handler);
	set_interrupt_handler_extra_data(pit->interrupt_vector, (void*)pit);

	const uint16_t counter_value = get_counter_value_from_frequency(target_frequency_hz);
	pit->interrupt_frequency = get_frequency_hz_from_counter_value(counter_value);//get the real frequency

	set_mode2_on_channel0(pit, counter_value);
	enable_PIT_irq(pit);

	return pit->interrupt_frequency;
}

inline uint64_t get_PIT_tick_count(PIT* pit){
	uint64_t ticks = 0;
	InterruptState istate = disable_and_save_interrupts();
	ticks = pit->tick_counter;
	restore_interrupt_state(istate);
	return ticks;
}

inline uint64_t get_ms_from_tick_count(PIT* pit, uint64_t tick){
	return (tick*1000)/pit->interrupt_frequency;
}

inline uint64_t get_us_from_tick_count(PIT* pit, uint64_t tick){
	return (tick*1000000)/pit->interrupt_frequency;
}

void PIT_wait_ms(PIT* pit, uint64_t ms){
	//interrupts MUST be enabled for the wait to work
	KASSERT(are_interrupts_enabled());
	uint64_t starting_ticks = get_PIT_tick_count(pit);
	while(get_ms_from_tick_count(pit, get_PIT_tick_count(pit) - starting_ticks) < ms)
		halt();//wait till next interrupt
	return;
}

void PIT_wait_us(PIT* pit, uint64_t ms){
	//interrupts MUST be enabled for the wait to work
	KASSERT(are_interrupts_enabled());
	uint64_t starting_ticks = get_PIT_tick_count(pit);
	while(get_us_from_tick_count(pit, get_PIT_tick_count(pit) - starting_ticks) < ms)
		halt();//wait till next interrupt
	return;
}

void PIT_interrupt_handler(InterruptInfo info){
	InterruptState istate = disable_and_save_interrupts();
	PIT* pit = (PIT*)info.extra_data;
	pit->tick_counter++;
	restore_interrupt_state(istate);
	return;
}
