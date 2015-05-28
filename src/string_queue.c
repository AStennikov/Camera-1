#include <string.h>
#include "string_queue.h"
#include <stdlib.h>
//#include "debug.h"


//initializes struct to default values
void string_queue_init(string_queue_t *queue/*, int length*/){
	queue->max_length = 30;//length;
	//queue->entry = malloc(sizeof(char)*queue->max_length);
	queue->length = 0;
	queue->first_entry = 0;
	queue->last_entry = 0;
}
 
//increments pointer. When pointer gets to last array element, resets pointer to the first element
void increment_pointer(string_queue_t *queue, int *pointer) {
	*pointer += 1;
	if (*pointer >= queue->max_length) {
		pointer = 0;
	}
	/*debug_uart_enable();
	sprintf(dbg, "first: %d, last: %d", queue->first_entry, queue->last_entry);
	debug(dbg);
	gsm_uart_enable();*/
	
}

int string_queue_size(string_queue_t *queue){
	return queue->length;
}

void string_enqueue(string_queue_t *queue, char *message){
	if (queue->length < queue->max_length) {
		char *new_entry = (char*) malloc(sizeof(char)*strlen(message));
		strcpy(new_entry, message);
		queue->entry[queue->last_entry] = new_entry;
		//queue->entry[queue->last_entry] = (char*) malloc(sizeof(char)*strlen(message));
		//strcpy(queue->entry[queue->last_entry], message);
		
		/*debug_uart_enable();
		sprintf(dbg, "enq: %s", queue->entry[queue->last_entry]);
		debug(dbg);
		gsm_uart_enable();*/
		
		increment_pointer(queue, &queue->last_entry);
		++queue->length;
	}
}

void string_dequeue(string_queue_t *queue, char *returned_string){
	if (queue->length > 0) {
		strcpy(returned_string, queue->entry[queue->first_entry]);
		free(queue->entry[queue->first_entry]);
		increment_pointer(queue, &queue->first_entry);
		--queue->length;
		
		/*debug_uart_enable();
		sprintf(dbg, "deq: %s", returned_string);
		debug(dbg);
		gsm_uart_enable();*/
	}
}

//removes all strings in queue
void string_queue_clear(string_queue_t *queue){
	while(queue->length > 0) {
		free(queue->entry[queue->first_entry]);
		increment_pointer(queue, &queue->first_entry);
		--queue->length;
	}
	/*while (string_queue_size(queue) > 0){
		if (queue->first_entry != NULL) {
        string_queue_entry_t *removed_entry = queue->first_entry;  			//stores address of removed entry
        queue->first_entry = removed_entry->next_entry;                 //updates queue first entry pointer
        free(removed_entry->string);                                    //removes entry
        free(removed_entry);
        --queue->number_of_entries;
    } else {
        queue->number_of_entries = 0;
    }
	}*/
}

/*void print_queue(string_queue_t *queue){
	//debug_uart_enable();
	//sprintf(dbg, "Queue Length: %d, start: %d, end: %d", queue->length, queue->first_entry, queue->last_entry);
	//debug(dbg);
	int i = 0;
	for (i=0; i<queue->max_length; ++i) {
		//debug(queue->entry[i]);
	}
}*/


/*//initializes struct to default values
void string_queue_init(string_queue_t *queue){
	queue->number_of_entries = 0;
	queue->is_busy = 0;
	queue->first_entry = NULL;
	queue->last_entry = NULL;
	queue->buffer_has_something = 0;
}

int string_queue_size(string_queue_t *queue){
	return queue->number_of_entries;
}

void string_enqueue(string_queue_t *queue, char *message){
	if (queue->is_busy == 0) {
		queue->is_busy = 1;
	
    //create new queue entry
    int message_length = (int) strlen(message);
    string_queue_entry_t *new_entry = malloc(sizeof(string_queue_entry_t));
    new_entry->string = malloc(message_length*sizeof(char));
    
    //fill entry with data
    strcpy(new_entry->string, message);
    new_entry->next_entry = NULL;
    
    //link this entry to previous entry
    if (queue->number_of_entries == 0) {
        queue->first_entry = new_entry;
    } else {
        queue->last_entry->next_entry = new_entry;
    }
    
    //update queue last entry pointer
    queue->last_entry = new_entry;
    
    ++queue->number_of_entries;
		
		queue->is_busy = 0;
	} else {
		strcpy(queue->buffer, message);
		queue->buffer_has_something = 1;
	}
}

void string_dequeue(string_queue_t *queue, char *returned_string){
	if (queue->is_busy == 0) {
		queue->is_busy = 1;
		
    if (queue->first_entry != NULL) {
        strcpy(returned_string, queue->first_entry->string);            //copies returned string
        string_queue_entry_t *removed_entry = queue->first_entry;  			//stores address of removed entry
        queue->first_entry = removed_entry->next_entry;                 //updates queue first entry pointer
        free(removed_entry->string);                                    //removes entry
        free(removed_entry);
        --queue->number_of_entries;
    } else {
        returned_string = NULL;
        queue->number_of_entries = 0;
    }
		queue->is_busy = 0;
	}
	
	//if something got stored in buffer, get it into queue
	if (queue->buffer_has_something == 1) {
		string_enqueue(queue, queue->buffer);
		queue->buffer_has_something = 0;
	}
	
	
}

//removes all strings in queue
void string_queue_clear(string_queue_t *queue){
	while (string_queue_size(queue) > 0){
		if (queue->first_entry != NULL) {
        string_queue_entry_t *removed_entry = queue->first_entry;  			//stores address of removed entry
        queue->first_entry = removed_entry->next_entry;                 //updates queue first entry pointer
        free(removed_entry->string);                                    //removes entry
        free(removed_entry);
        --queue->number_of_entries;
    } else {
        queue->number_of_entries = 0;
    }
	}
}

void print_queue(string_queue_t *queue){
	debug_uart_enable();
	string_queue_entry_t *pointer = queue->first_entry;
	int j = 1;
	while (pointer != NULL) {
			sprintf(dbg, "%d of %d, address: %x, message: %s, next: %x\n", j, (int) queue->number_of_entries, (int) pointer, pointer->string, (int) pointer->next_entry);
			debug(dbg);
			pointer = pointer->next_entry;
			++j;
	}
}*/



