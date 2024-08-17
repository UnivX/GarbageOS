#pragma once
#include <stdatomic.h>
#include "../hal.h"

//hard spinlocks disable interrupts
typedef struct hard_spinlock{
	atomic_flag flag;
	InterruptState istate;
} hard_spinlock;

void init_hard_spinlock(hard_spinlock* s);
void acquire_hard_spinlock(hard_spinlock* s);
void release_hard_spinlock(hard_spinlock* s);

typedef atomic_flag soft_spinlock;

void init_soft_spinlock(soft_spinlock* s);
void acquire_soft_spinlock(soft_spinlock* s);
void release_soft_spinlock(soft_spinlock* s);

