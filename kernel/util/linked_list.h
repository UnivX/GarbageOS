#pragma once
#include "../kdefs.h"
#include "../mem/heap.h"

typedef struct LinkedListNode{
	void* data;
	struct LinkedListNode* next;
} LinkedListNode;

typedef struct LinkedList{
	LinkedListNode* head;
	uint64_t size;
} LinkedList;


LinkedList create_linked_list();
void linked_list_destroy(LinkedList* ll);

void linked_list_remove_after(LinkedList* ll, LinkedListNode* node);
void linked_list_remove_at(LinkedList* ll, uint64_t index);

void linked_list_insert_after(LinkedList* ll, LinkedListNode* node, void* data);
void linked_list_insert_at(LinkedList* ll, uint64_t index, void* data);

void linked_list_push_front(LinkedList* ll, void* data);
void linked_list_pop_front(LinkedList* ll);

LinkedListNode* linked_list_get_at(LinkedList ll, uint64_t index);
