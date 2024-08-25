#include "vaddr_cache_shootdown.h"
#include "../kernel_data.h"
#include "../interrupt/apic.h"
#include "heap.h"

struct VMMCacheShootdownSystem{
	volatile VMMCacheShootdownQueue* queue_array;
	uint64_t queues_count;
};
static struct VMMCacheShootdownSystem cache_shootdown_sys = {NULL, 0};

uint64_t get_logical_core_vmmcache_shootdown_queue_index(){
	KASSERT(cache_shootdown_sys.queue_array != NULL);
	CPUID cpuid = get_cpu_id_from_apic_id(get_logical_core_lapic_id());
	uint64_t index = (uint64_t)cpuid;
	KASSERT(index < cache_shootdown_sys.queues_count);
	return index;
}
volatile struct VMMCacheShootdownQueue* get_logical_core_vmmcache_shootdown_queue(){
	KASSERT(cache_shootdown_sys.queue_array != NULL);
	CPUID cpuid = get_cpu_id_from_apic_id(get_logical_core_lapic_id());
	uint64_t index = (uint64_t)cpuid;
	KASSERT(index < cache_shootdown_sys.queues_count);
	return &cache_shootdown_sys.queue_array[index];
}

void cache_shootdown_interrupt_handler(InterruptInfo info);

struct CSQNode* create_CSQNode(VMMCacheShootdownRange range){
		struct CSQNode* new_node = (struct CSQNode*)kmalloc(sizeof(struct CSQNode));
		new_node->next = NULL;
		new_node->range = range;
		return new_node;
}

void destroy_CSQNode(struct CSQNode* node){
	kfree(node);
}

inline bool unsync_is_shootdown_q_empty(volatile VMMCacheShootdownQueue* q){
	return q->q_head == NULL && q->q_tail == NULL;
}

inline bool is_shootdown_q_empty(volatile VMMCacheShootdownQueue* q){
	ACQUIRE_SPINLOCK_HARD(&q->queue_lock);
	bool result = unsync_is_shootdown_q_empty(q);
	RELEASE_SPINLOCK_HARD(&q->queue_lock);
	return result;
}

void push_to_shootdown_q(volatile VMMCacheShootdownQueue* q, VMMCacheShootdownRange range){
	InterruptState istate;
	acquire_spinlock_hard(&q->queue_lock, &istate);
	if(unsync_is_shootdown_q_empty(q)){
		//we have an empty queue
		struct CSQNode* new_node = create_CSQNode(range);
		q->q_head = new_node;
		q->q_tail = new_node;
	}else{
		//we have at least an element
		struct CSQNode* old_head = q->q_head;
		struct CSQNode* new_node = create_CSQNode(range);
		q->q_head = new_node;
		q->q_head->next = old_head;
	}
	q->node_count++;
	release_spinlock_hard(&q->queue_lock, &istate);
}

VMMCacheShootdownRange pop_from_shootdown_q(volatile VMMCacheShootdownQueue* q){
	VMMCacheShootdownRange range = {NULL,0};

	InterruptState istate;
	acquire_spinlock_hard(&q->queue_lock, &istate);
	KASSERT(!unsync_is_shootdown_q_empty(q));

	range = q->q_head->range;
	//check if we have only one element left
	//it cant be NULL == NULL because we have an assert before that checks if 
	//the queue is empty(q_head = NULL and q_tail = NULL)
	if(q->q_head == q->q_tail){
		//one element left
		destroy_CSQNode(q->q_head);
		q->q_head = NULL;
		q->q_tail = NULL;
	}else{
		//we have at least two elements
		struct CSQNode* to_pop = q->q_head;
		q->q_head = q->q_head->next;
		destroy_CSQNode(to_pop);
	}
	q->node_count--;

	release_spinlock_hard(&q->queue_lock, &istate);
	return range;
}

bool is_shootdown_q_active(volatile VMMCacheShootdownQueue* q){
	ACQUIRE_SPINLOCK_HARD(&q->queue_lock);
	bool result = q->active;
	RELEASE_SPINLOCK_HARD(&q->queue_lock);
	return result;
}

uint64_t get_shootdown_q_count(volatile VMMCacheShootdownQueue* q){
	ACQUIRE_SPINLOCK_HARD(&q->queue_lock);
	uint64_t result = q->node_count;
	RELEASE_SPINLOCK_HARD(&q->queue_lock);
	return result;
}

void delete_vmmcache_range(VMMCacheShootdownRange range){
	uint64_t start = (uint64_t)range.addr_start;
	start -= start % PAGE_SIZE;

	for(uint64_t i = 0; i < range.size_in_pages; i++){
		volatile void* vaddr = (void*)(start + i*PAGE_SIZE);
		delete_page_translation_cache(vaddr);
	}
}

void initialize_cache_shootdown_queue(volatile VMMCacheShootdownQueue* queue){
	queue->q_head = NULL;
	queue->q_tail = NULL;
	init_spinlock(&queue->queue_lock);
}

void initialize_vmmcache_shootdown_subsystem(){
	cache_shootdown_sys.queues_count = get_number_of_usable_logical_cores();
	cache_shootdown_sys.queue_array = kmalloc(sizeof(struct VMMCacheShootdownQueue)*cache_shootdown_sys.queues_count);
	for(uint64_t i = 0; i < cache_shootdown_sys.queues_count; i++)
		initialize_cache_shootdown_queue(&cache_shootdown_sys.queue_array[i]);

	install_interrupt_handler(VMMCACHE_SHOOTDOWN_VECTOR, cache_shootdown_interrupt_handler);
}

void activate_this_cpu_vmmcache_shootdown(){
	volatile VMMCacheShootdownQueue* queue = get_logical_core_vmmcache_shootdown_queue();
	ACQUIRE_SPINLOCK_HARD(&queue->queue_lock);
	queue->active = true;
	RELEASE_SPINLOCK_HARD(&queue->queue_lock);
	//boom the vmm cache
	//we do this because we may have lost some VMMCache shootdowns
	delete_all_translation_cache();
}

void enqueue_vmmcache_shootdown_range(VMMCacheShootdownRange range){
	//add the range to the queues and then send an Inter Processor Interrupt(IPI)
	KASSERT(cache_shootdown_sys.queue_array != NULL);
	//get the index of the queue for this logical core
	uint64_t self_queue_index = get_logical_core_vmmcache_shootdown_queue_index();
	for(uint64_t i = 0; i < cache_shootdown_sys.queues_count; i++){
		volatile VMMCacheShootdownQueue* q = &cache_shootdown_sys.queue_array[i];
		//the queue cannot be deactivated once active
		//so we do not have any critical race condition
		//we have a race condition where we may lose some shootdowns
		//this is taken care of in activate_this_cpu_vmmcache_shootdown
		if(is_shootdown_q_active(q) && i != self_queue_index)
			push_to_shootdown_q(q, range);
	}
}

void empty_all_vmmcache_shootdown_queues(){
	send_IPI_by_destination_shorthand(APIC_DESTSH_ALL_INCLUDING_SELF, VMMCACHE_SHOOTDOWN_VECTOR);
}

void empty_other_cpus_vmmcache_shootdown_queues(){
	send_IPI_by_destination_shorthand(APIC_DESTSH_ALL_EXCLUDING_SELF, VMMCACHE_SHOOTDOWN_VECTOR);
}

void vmmcache_shootdown(VMMCacheShootdownRange range){
	enqueue_vmmcache_shootdown_range(range);
	empty_other_cpus_vmmcache_shootdown_queues();
}

void cache_shootdown_interrupt_handler(InterruptInfo info){
	//InterruptState istate = disable_and_save_interrupts();
	volatile VMMCacheShootdownQueue* queue = get_logical_core_vmmcache_shootdown_queue();
	uint32_t cpuid = (uint32_t)get_cpu_id_from_apic_id(get_logical_core_lapic_id());
#ifdef PRINT_ALL_VMMCACHE_SHOOTDOWNS
	if(is_kio_initialized())
		printf("emptying VMM cache shootdown queue with cpuid=%u32, queue count=%u64\n", cpuid, get_shootdown_q_count(queue));
#endif
	while(!is_shootdown_q_empty(queue)){
		VMMCacheShootdownRange range = pop_from_shootdown_q(queue);
		delete_vmmcache_range(range);
	}
	//restore_interrupt_state(istate);
}

bool is_vmmcache_shootdown_subsystem_initialized(){
	return cache_shootdown_sys.queue_array != NULL;
}
