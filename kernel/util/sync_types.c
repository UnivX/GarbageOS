#include "sync_types.h"

inline void init_spinlock(spinlock* s){
	atomic_flag_clear_explicit(s, memory_order_release);
}

inline void acquire_spinlock(spinlock* s){
	while(atomic_flag_test_and_set_explicit(s, memory_order_acquire))
        __builtin_ia32_pause();
	return;
}

inline void release_spinlock(spinlock* s){
	atomic_flag_clear_explicit(s, memory_order_release);
	return;
}

void acquire_spinlock_hard(spinlock* s, InterruptState* istate){
	*istate = disable_and_save_interrupts();
	while(atomic_flag_test_and_set_explicit(s, memory_order_acquire)){
		restore_interrupt_state(*istate);
        __builtin_ia32_pause();
		*istate = disable_and_save_interrupts();
	}
	return;
}

void release_spinlock_hard(spinlock* s, InterruptState* istate){
	atomic_flag_clear_explicit(s, memory_order_release);
	restore_interrupt_state(*istate);
	return;
}
