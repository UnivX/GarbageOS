#include "linked_list.h"

LinkedList create_linked_list(){
	LinkedList ll;
	ll.head = NULL;
	ll.size = 0;
	return ll;
}

void linked_list_destroy(LinkedList* ll){
	LinkedListNode* node = ll->head;
	while(node != NULL){
		LinkedListNode* next = node->next;
		kfree(node);
		node = next;
	}
	ll->size = 0;
	ll->head = NULL;
}

void linked_list_remove_after(LinkedList *ll, LinkedListNode* node){
	KASSERT(ll->head != NULL && ll->size != 0 && node != NULL);
	KASSERT(node->next != NULL);
	LinkedListNode* new_next = node->next->next;
	kfree(node->next);
	node->next = new_next;
	ll->size--;

}
void linked_list_remove_at(LinkedList *ll, uint64_t index){
	KASSERT(ll->head != NULL && ll->size != 0);

	if(index == 0){
		linked_list_pop_front(ll);
		return;
	}

	LinkedListNode* prev = ll->head;
	for(uint64_t i = 0; i < index-1; i++){
		KASSERT(prev != NULL);
		prev = prev->next;
	}
	linked_list_remove_after(ll, prev);
}

void linked_list_insert_after(LinkedList* ll, LinkedListNode* node, void* data){
	KASSERT(ll->head != NULL && ll->size != 0 && node != NULL);
	LinkedListNode* new_node = kmalloc(sizeof(LinkedListNode));
	new_node->data = data;
	new_node->next = node->next;
	node->next = new_node;
	ll->size++;
}
void linked_list_insert_at(LinkedList* ll, uint64_t index, void* data){
	if(index == 0){
		linked_list_push_front(ll, data);
		return;
	}

	LinkedListNode* prev = ll->head;
	for(uint64_t i = 0; i < index-1; i++){
		KASSERT(prev != NULL);
		prev = prev->next;
	}
	linked_list_insert_after(ll, prev, data);
}

void linked_list_push_front(LinkedList* ll, void* data){
	LinkedListNode* new_node = kmalloc(sizeof(LinkedListNode));
	new_node->data = data;
	new_node->next = ll->head;
	ll->head = new_node;
	ll->size++;
}
void linked_list_pop_front(LinkedList* ll){
	KASSERT(ll->head != NULL && ll->size != 0);
	LinkedListNode* old_node = ll->head;
	ll->head = ll->head->next;
	kfree(old_node);
	ll->size--;
}

LinkedListNode* linked_list_get_at(LinkedList ll, uint64_t index){
	LinkedListNode* node = ll.head;
	for(uint64_t i = 0; i < index-1; i++){
		KASSERT(node != NULL);
		node = node->next;
	}
	return node;
}
