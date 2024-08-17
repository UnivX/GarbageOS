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
