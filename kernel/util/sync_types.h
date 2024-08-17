#pragma once
#include <stdatomic.h>

typedef atomic_flag spinlock;

void init_spinlock(spinlock* s);
void acquire_spinlock(spinlock* s);
void release_spinlock(spinlock* s);

