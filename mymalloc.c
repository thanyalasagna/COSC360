/*Thanya Nguyen Lab 7 Malloc
4/2/2024  This program mimics a malloc
function and freeing function. This uses
coalescing and to make sure the memory managemnt is good.
Help from TA maria, adam, and jacob!*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mymalloc.h"

typedef struct chunk {
	//Size is used for both allocated and free frames
	int size; //how much memory was given to u when u asked for it
	struct chunk *flink;
	struct chunk *blink;
} chunk;

void connect(chunk *from, chunk *to){
	from->flink = to;
	to->blink = from;
}

struct chunk *head = NULL;

void proper_insert(chunk *ptr){
	// printf("properInsert()\n");
	chunk *prev, *tmp;
	chunk *p =ptr;
		/*using loop to be able to find spot to insert correctly*/
	tmp = head;

	
	if (head == NULL) { //if list is empty
		// printf("list is empty\n");
		head = ptr;
		return;
	}

	//printf("going into while loop\n");
	while(tmp != NULL){
		prev = tmp;
		tmp = tmp->flink;
		if(p < tmp && p > prev){
			break;
		}
	}

	if(prev->flink != NULL){ 
		connect(p, tmp);
	}
	connect(prev, p); //if this is at the end of the list
	return;
}

void print_list(){
	printf("\n\n PRINT LIST:\n");
	chunk *tmp;
	tmp = head;
	int counter = 0; 
	while(tmp != NULL){
		printf("Node %d: %p\n   size = %d\n   flink = %p\n   blink = %p\n", counter,tmp, tmp->size, tmp->flink,tmp->blink);
		counter++;
		tmp= tmp->flink;
	}
	return;
}

//loop n find a chunk size that is >= what u req
chunk* traverse(size_t req_size){ 
	// printf("traverse()\n");
	chunk *tmp;
	tmp = head;

	while(tmp != NULL){
		// printf("node size = %d\n", tmp->size);
		if(tmp->size >= req_size){
			return tmp;
		}
		tmp = tmp->flink;

	}
	return NULL;
}

void *delete_node(void *ptr){
	chunk *node = ptr;


	if(node != NULL){
		//checking to prevent segfaults
		if(node->flink != NULL){
			if(node->flink->blink != NULL){
				node->flink->blink = node->blink;
			}
		}
		//checking to prevent seg faults
		if(node->blink != NULL){
			if(node->blink->flink != NULL){
				node->blink->flink = node->flink;
			}
		}
	}
	if(ptr == head){
		head = node->flink;
	}
	return ptr;
}

void *my_malloc(size_t size) {
	chunk *free_chunk;
	size_t new_size = (size + 7 + 8) & -8; // Adjust for alignment and metadata

	// printf("Requesting: %lu, Aligned: %lu\n", size, new_size);

	// Find a suitable chunk
	free_chunk = traverse(new_size);
	// printf("free chunk check = %p\n", free_chunk);
	if (!free_chunk) { //if i need to make a chunk, insert to list
		// printf("chunk DNE! making another one\n");
		size_t allocate_size = new_size > 8192 ? new_size : 8192;
		free_chunk = sbrk(allocate_size);
		if (free_chunk == (void*)-1) {
			return NULL; // sbrk failed
		}

		free_chunk->size = allocate_size;
		free_chunk->flink = NULL;
		free_chunk->blink = NULL;

		proper_insert(free_chunk);

	}

	if(free_chunk->size - new_size <= 16){ //return the whole chunk, dont split
		void *give_to_user = delete_node(free_chunk);
		void *user_memory = (void *)(give_to_user + 8); //void means +8
		return user_memory;
	}

	//breaks off chunk, changes the sizes properly
	chunk *next = (chunk *)((char *)free_chunk + free_chunk->size - new_size);
 
	next->size = new_size; //
	free_chunk->size -=  new_size;


	void *give_to_user = delete_node(next);
	//printf("next size = %d  addy = %p", next->size, next);
	//print_list();


void *user_memory = (void *)(give_to_user + 8); //void means +8

//printf("Allocated memory at %p\n", user_memory);
return user_memory;

}


void my_free(void *ptr){
	chunk *prev, *tmp;
	chunk *p = ptr - 8; // maybe keep this idk
	//chunk *p = ptr - sizeof(chunk);

	p->flink = NULL;
	p->blink = NULL;

	if(head == NULL){ //you allocated everything already, no free list, nothing is free
		head = p;
		return;
	}

	//1. Back up by 8 bytes.
	//2. Add this to the free list
	if(p < head){ //add before the head
		connect(p, head);
		head = p;
		return;
	}

	/*using loop to be able to find spot to insert correctly*/
	tmp = head;
	while(tmp != NULL){
		prev = tmp;
		tmp = tmp->flink;
		if(p < tmp && p > prev){
			break;
		}
	}

	if(prev->flink != NULL){ 
		connect(p, tmp);
	}
	connect(prev, p); //if this is at the end of the list


}

void *free_list_begin(){
	//The head or NULL if there is no head
	struct chunk *ptr;
	ptr = head;
	return ptr;
}

void *free_list_next(void *ptr){
	//ptr->next or NULL if there isn't a next
	//If you use a doubly-linked list, be careful
	//here or you'll have an infinite loop!
	chunk *c = (chunk *) ptr;

	if(c->flink == NULL){
		return NULL;
	}
	return c->flink;
}
/*help from maria, traversing thru the function
and merging the blocks*/
void coalesce_free_list(){
	chunk *tmp;
	tmp = head;

	while(tmp != NULL){
		if((void *)(tmp) + tmp->size == tmp->flink){ //blocks r adjacent
			tmp->size += tmp->flink->size;
			delete_node(tmp->flink);
		}
		else{
			tmp = tmp->flink;
		}
	}
}