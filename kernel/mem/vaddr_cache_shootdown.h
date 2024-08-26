/* DEPENDS ON: interrupt subsystem
 * DEPENDS ON: Local APIC functionalities
 * DEPENDS ON: kernel_data.h get_cpu_id_from_apic_id() and the previous call to register_local_kernel_data_cpu()
 * DEPENDS ON: kmalloc, kfree
 *
 * We need to ensure cache coherency for the virtual addresses translation caches
 * in the x86/x86_64 architectures we are talking about the TLB(Translation Lookaside Buffer)
 * wich isn't syncronized beetwen cpu by the hardware
 * these function are primary called from the Virtual Memory Manager(VMM)
 */

#pragma once
#include "../kdefs.h"
#include "../hal.h"
#include "../util/sync_types.h"

//This structure is used for notifying the processor that has executed the 
//emptying of all cpus' queue when the shootdown has finished
//internaly the pointer to the state is a value of the elements pushed to the pending shootdowns queue.
//When the element is popped from the queue if the pointer is not NULL the state will be updated
typedef struct VMMCacheShootdownState{
	unsigned int queues_to_wait_count;
	atomic_uint emptied_queues_count;
} VMMCacheShootdownState;

void wait_for_vmmcache_shootdown_completition(VMMCacheShootdownState *state);

typedef struct VMMCacheShootdownRange{
	void* addr_start;
	uint64_t size_in_pages;
} VMMCacheShootdownRange;

#define NULL_VMMCACHE_SHOOTDOWN_RANGE {NULL, 0}

void activate_this_cpu_vmmcache_shootdown();
void initialize_vmmcache_shootdown_subsystem();

//enqueue the shootdown of a range of virtual addresses translation cache
//without executing the shootdown
void enqueue_vmmcache_shootdown_range(VMMCacheShootdownRange range);

//empty all the queues(one per cpu) of pending shootdowns of 
//virtual addresses translation cache
//this function empty also the queue of this CPU
//state is used to know the state of the shootdown, it's a pointer to the struct allocated by the caller.
//state may be null if you do not need it
//DO NOT DEALLOCATE STATE BEFORE CALLING wait_for_vmmcache_shootdown_completition
void empty_all_vmmcache_shootdown_queues(VMMCacheShootdownState* state);
//empty all the queues(one per cpu), except the caller queue, of pending shootdowns of 
//virtual addresses translation cache
//this function empty also the queue of this CPU
//state is used to know the state of the shootdown, it's a pointer to the struct allocated by the caller.
//state may be null if you do not need it
//DO NOT DEALLOCATE STATE BEFORE CALLING wait_for_vmmcache_shootdown_completition
void empty_other_cpus_vmmcache_shootdown_queues(VMMCacheShootdownState* state);

//shootdown of the virtual addresses translation caches of the range
//state is used to know the state of the shootdown, it's a pointer to the struct allocated by the caller.
//state may be null if you do not need it
//DO NOT DEALLOCATE STATE BEFORE CALLING wait_for_vmmcache_shootdown_completition
void vmmcache_shootdown(VMMCacheShootdownRange range, VMMCacheShootdownState* state);

bool is_vmmcache_shootdown_subsystem_initialized();
