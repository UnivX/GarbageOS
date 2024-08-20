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

#define SPINLOCK_HARD_TO_SOFT(s) \
	restore_interrupt_state(spinlock_istate)

#define SPINLOCK_HARD_FROM_SOFT(s) \
	spinlock_istate = disable_and_save_interrupts

#define SPINLOCK_HARD_ISTATE spinlock_istate

void init_spinlock(volatile spinlock* s);
void acquire_spinlock(volatile spinlock* s);
void release_spinlock(volatile spinlock* s);

void acquire_spinlock_hard(volatile spinlock* s, InterruptState* istate);
void release_spinlock_hard(volatile spinlock* s, InterruptState* istate);
