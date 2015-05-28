#ifndef string_queue_h
#define string_queue_h


//single string
/*typedef struct {
    char *string;
} string_queue_entry_t;*/

//queue of strings
typedef struct {
    int first_entry;
    int last_entry;
    int length;			//current length
	int max_length;	//maximum allowed length
	char *entry[30];
    //int is_busy;           //indicates that user is writing or reading queue. Interrupts must delay their action if is_busy is not 0.
		//int buffer_has_something;
		//char buffer[128];
} string_queue_t;

//initializes struct to default values
void string_queue_init(string_queue_t *queue/*, int length*/);

int string_queue_size(string_queue_t *queue);

void string_enqueue(string_queue_t *queue, char *message);

void string_dequeue(string_queue_t *queue, char *returned_string);

//removes all strings in queue
void string_queue_clear(string_queue_t *queue);

//for debug
//void print_queue(string_queue_t *queue);


/*//single string
typedef struct string_queue_entry_t string_queue_entry_t;
struct string_queue_entry_t{
    char *string;
    string_queue_entry_t *next_entry;
};

//queue of strings
typedef struct {
    string_queue_entry_t *first_entry;
    string_queue_entry_t *last_entry;
    int number_of_entries;
    int is_busy;           //indicates that user is writing or reading queue. Interrupts must delay their action if is_busy is not 0.
		int buffer_has_something;
		char buffer[128];
} string_queue_t;

//initializes struct to default values
void string_queue_init(string_queue_t *queue);

int string_queue_size(string_queue_t *queue);

void string_enqueue(string_queue_t *queue, char *message);

void string_dequeue(string_queue_t *queue, char *returned_string);

//removes all strings in queue
void string_queue_clear(string_queue_t *queue);

//for debug
void print_queue(string_queue_t *queue);*/

#endif
