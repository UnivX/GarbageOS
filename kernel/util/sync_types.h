#pragma once
#include <stdatomic.h>
#include "../hal.h"

typedef atomic_flag spinlock;

/*
#define ACQUIRE_SPINLOCK_HARD(s) \
	InterruptState spinlock_istate; \
	spinlock_istate = disable_and_save_interrupts(); \
	acquire_spinlock(s)

#define RELEASE_SPINLOCK_HARD(s) \
	release_spinlock(s); \
	restore_interrupt_state(spinlock_istate)
*/

#define ACQUIRE_SPINLOCK_HARD(s) \
	InterruptState spinlock_istate; \
	acquire_spinlock_hard(s, &spinlock_istate)

#define RELEASE_SPINLOCK_HARD(s) \
	release_spinlock_hard(s, &spinlock_istate); \

void init_spinlock(spinlock* s);
void acquire_spinlock(spinlock* s);
void release_spinlock(spinlock* s);

void acquire_spinlock_hard(spinlock* s, InterruptState* istate);
void release_spinlock_hard(spinlock* s, InterruptState* istate);
