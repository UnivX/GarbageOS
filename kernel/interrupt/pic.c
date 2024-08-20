#include "pic.h"
#include "../util/sync_types.h"


spinlock pic_lock;

void pic_ack_interrupt(uint64_t interrupt_number, bool spurious){
	ACQUIRE_SPINLOCK_HARD(&pic_lock);
	if(spurious){
		//if it's the slave spurious interrupt
		//then send the ack to the master
		if(interrupt_number - PIC_IDT_START == 15){
			outb(MASTER_COMMAND_PORT, PIC_EOI);
		}
	}else{
		if(interrupt_number - PIC_IDT_START >= 8)
			outb(SLAVE_COMMAND_PORT, PIC_EOI);

		outb(MASTER_COMMAND_PORT, PIC_EOI);
	}
	RELEASE_SPINLOCK_HARD(&pic_lock);
}

void init_pic(){
	init_spinlock(&pic_lock);
	//first initialization command
	//ICW1_ICW4 tells the pic that we will send an ICW4 command
	outb(MASTER_COMMAND_PORT, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(SLAVE_COMMAND_PORT, ICW1_INIT | ICW1_ICW4);
	io_wait();

	//set the base interrupt number(offset) (ICW2)
	outb(MASTER_DATA_PORT, MASTER_OFFSET);
	io_wait();
	outb(SLAVE_DATA_PORT, SLAVE_OFFSET);
	io_wait();
	
	//ICW3 tell the pic which irq is used to communicate(spurius irq)
	//in x86 the pics are connected via irq 2
	io_wait();
	outb(MASTER_DATA_PORT, 4);//irq n 2 -> 2^2 = 4
	io_wait();
	outb(SLAVE_DATA_PORT, 2);//irq n 2
	io_wait();

	outb(MASTER_DATA_PORT, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	outb(SLAVE_DATA_PORT, ICW4_8086);
	io_wait();

	//set masks as all 1
	outb(SLAVE_DATA_PORT, 0xFF);
	outb(MASTER_DATA_PORT, 0xFF);
}

void set_pic_irq_mask(uint8_t irq_line){
	irq_line -= PIC_IDT_START;

	uint16_t port;
    if(irq_line < 8)
        port = MASTER_DATA_PORT;
    else {
        port = SLAVE_DATA_PORT;
        irq_line -= 8;
    }
	ACQUIRE_SPINLOCK_HARD(&pic_lock);
    uint8_t value = inb(port) | (1 << irq_line);
    outb(port, value);
	RELEASE_SPINLOCK_HARD(&pic_lock);
}

void clear_pic_irq_mask(uint8_t irq_line){
	irq_line -= PIC_IDT_START;

	uint16_t port;
    if(irq_line < 8)
        port = MASTER_DATA_PORT;
    else {
        port = SLAVE_DATA_PORT;
        irq_line -= 8;
    }
	ACQUIRE_SPINLOCK_HARD(&pic_lock);
    uint8_t value = inb(port) & ~(1 << irq_line);
    outb(port, value);
	RELEASE_SPINLOCK_HARD(&pic_lock);
}

uint16_t get_pic_isr(){
	ACQUIRE_SPINLOCK_HARD(&pic_lock);
	outb(MASTER_COMMAND_PORT, PIC_READ_ISR);
    outb(SLAVE_COMMAND_PORT, PIC_READ_ISR);
    uint16_t result = (inb(SLAVE_COMMAND_PORT) << 8) | inb(SLAVE_COMMAND_PORT);
	RELEASE_SPINLOCK_HARD(&pic_lock);
	return result;
}

bool is_pic_interrupt_spurious(uint8_t interrupt_number){
	uint8_t irq = interrupt_number-PIC_IDT_START;
	if (irq == 7){
		return (get_pic_isr() & (1 << irq)) == 0;
	}else if (irq == 15){

	}
	return false;
}

void disable_pic() {
	//set masks as all 1
	ACQUIRE_SPINLOCK_HARD(&pic_lock);
	outb(SLAVE_DATA_PORT, 0xFF);
	outb(MASTER_DATA_PORT, 0xFF);
	RELEASE_SPINLOCK_HARD(&pic_lock);
}
