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
//TODO: test the shootdown

#pragma once
#include "../kdefs.h"
#include "../hal.h"
#include "../util/sync_types.h"
#define PRINT_ALL_VMMCACHE_SHOOTDOWNS

//TODO: make it fr
//this structure is used for notifying the processor that has executed the 
//emptying of all cpus' queue when the shootdown has finished
typedef struct VMMCacheShootdownState{
	uint64_t queues_to_wait_count;
} ShootdownQueueEmptyCounter;

typedef struct VMMCacheShootdownRange{
	void* addr_start;
	uint64_t size_in_pages;
} VMMCacheShootdownRange;

typedef struct VMMCacheShootdownQueue{
	struct CSQNode{
		VMMCacheShootdownRange range;
		struct CSQNode* next;
	};
	struct CSQNode *q_head;
	struct CSQNode *q_tail;
	uint64_t node_count;
	//the queue cannot be deactivated
	//this makes the concurrency management easier
	bool active;
	spinlock queue_lock;
} VMMCacheShootdownQueue;

void activate_this_cpu_vmmcache_shootdown();
void initialize_vmmcache_shootdown_subsystem();

//enqueue the shootdown of a range of virtual addresses translation cache
//without executing the shootdown
void enqueue_vmmcache_shootdown_range(VMMCacheShootdownRange range);

//empty all the queues(one per cpu) of pending shootdowns of 
//virtual addresses translation cache
//this function empty also the queue of this CPU
void empty_all_vmmcache_shootdown_queues();
//empty all the queues(one per cpu), except the caller queue, of pending shootdowns of 
//virtual addresses translation cache
//this function empty also the queue of this CPU
void empty_other_cpus_vmmcache_shootdown_queues();

//shootdown of the virtual addresses translation caches of the range
void vmmcache_shootdown(VMMCacheShootdownRange range);

bool is_vmmcache_shootdown_subsystem_initialized();
