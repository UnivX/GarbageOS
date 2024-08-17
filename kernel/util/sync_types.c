#include "sync_types.h"
inline void init_hard_spinlock(hard_spinlock* s){
	atomic_flag_clear_explicit(&s->flag, memory_order_release);
}

inline void acquire_hard_spinlock(hard_spinlock* s){
	s->istate = disable_and_save_interrupts();
	while(atomic_flag_test_and_set_explicit(&s->flag, memory_order_acquire)){
		restore_interrupt_state(s->istate);
        __builtin_ia32_pause();
		s->istate = disable_and_save_interrupts();
	}
	return;
}

inline void release_hard_spinlock(hard_spinlock* s){
	atomic_flag_clear_explicit(&s->flag, memory_order_release);
	restore_interrupt_state(s->istate);
	return;
}

inline void init_soft_spinlock(soft_spinlock* s){
	atomic_flag_clear_explicit(s, memory_order_release);
}

inline void acquire_soft_spinlock(soft_spinlock* s){
	while(atomic_flag_test_and_set_explicit(s, memory_order_acquire))
        __builtin_ia32_pause();
	return;
}

inline void release_soft_spinlock(soft_spinlock* s){
	atomic_flag_clear_explicit(s, memory_order_release);
	return;
}
