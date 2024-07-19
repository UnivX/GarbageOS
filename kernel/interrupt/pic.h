#pragma once
#include "../hal.h"
#include "../kdefs.h"

#define MASTER_COMMAND_PORT 0x0020
#define MASTER_DATA_PORT 0x0021

#define SLAVE_COMMAND_PORT 0x00A0
#define SLAVE_DATA_PORT 0x00A1

#define PIC_IDT_START 0x20
#define PIC_IDT_SIZE 16

#define MASTER_OFFSET PIC_IDT_START
#define SLAVE_OFFSET MASTER_OFFSET+8


//end of interrupt code
#define PIC_EOI 0x20


#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
 
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

#define PIC_READ_ISR                0x0B

void pic_ack_interrupt(uint64_t interrupt_number, bool spurious);
void init_pic();
void disable_pic();
void set_pic_irq_mask(uint8_t irq_line);
void clear_pic_irq_mask(uint8_t irq_line);
//combined bit map of the serviced irq_lines
uint16_t get_pic_isr();
bool is_pic_interrupt_spurious(uint8_t interrupt_number);
