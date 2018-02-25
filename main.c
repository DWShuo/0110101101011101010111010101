#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "heapq.h"

#define MAXLINE 100
#define MAXSIZE 26

//EVENT AND PROCESS STRUCTS
char *scheduler_name[3] = {"FCFS", "SRT", "RR"};

typedef enum{
    FCFS = 0,
    SRT = 1,
    RR = 2
} scheduler_t;

struct _event_t;
typedef struct {
	char id;    			 //   ]
	int32_t init_arrival_time;//  ] same thing as enqueue time?
	int32_t cpu_burst_time;  //   ]-- from the input file
	int32_t cpu_burst_count; //   ]
	int32_t io_burst_time;   //   ]
	int32_t enqueue_time;  // the time this process entered the ready queue
	// for SRT/RR
	int32_t start_time;  // latest time the process entered the running state
	int32_t remaining_burst_time;
	struct _event_t *context_switch_out;  // context switch-out event for this process, cancelled during preemption
	struct _event_t *time_slice;  // time slice event for this process, cancelled when the cpu burst ends
} process_t;

typedef enum {
	EVENT_CONTEXT_SWITCH_OUT = -1,
	EVENT_IO_FINISHED,
	EVENT_PROCESS_ARRIVAL,
	EVENT_CONTEXT_SWITCH_IN,
	EVENT_TIME_SLICE
} event_type;

struct _event_t {
	event_type type;
	int32_t time;  // the time when this event should happen
	process_t *process;  // process associated with this event
	int cancelled;  //  if this event is cancelled
};
typedef struct _event_t event_t;

//SIMULATOR AND STATISTIC VARIABLES
// simulator global variables and parameters
int32_t current_time = 0;
process_t *current_process = NULL;
heapq_t eventq[1];
heapq_t readyq[1];
int rr_add = 0;
int t_cs = 8;
int t_slice = 80;
// statistics
int32_t total_burst_time;
int32_t burst_count;
int32_t total_wait_time;
int32_t wait_count;
int32_t total_turnaround_time;
int32_t turnaround_count;
int32_t context_switches;
int32_t preemptions;

//COMPARISON FUNCTIONS
int cmp_fcfs(void *_p1, void *_p2);
int cmp_srt(void *_p1, void *_p2);
int cmp_rr(void *_p1, void *_p2);


//NOTE: HEAP HELPER FUNCTIONS
//EVENT HELPERS
event_t *create_event(event_type type, int32_t time, process_t *proc) {
	event_t *event = (event_t *)calloc(sizeof(event_t), 1);
	event->type = type;
	event->time = time;
	event->process = proc;
	return event;
}
void event_push(event_t *event) {
	heap_push(eventq, event);
}
event_t *event_pop() {
	return eventq->length > 0 ? (event_t *)heap_pop(eventq) : NULL;
}
event_t *event_peek() {
	return eventq->length > 0 ? (event_t *)eventq->elements[0] : NULL;
}

//READY Q HELPERS
void ready_push(process_t *proc, int beginning) {
	// for rr_add = 1, use a negative enqueue time so this process is placed on front
	proc->enqueue_time = beginning ? -current_time : current_time;
	total_wait_time -= current_time;
	wait_count++;
	heap_push(readyq, proc);
}
process_t *ready_pop() {
	if (readyq->length > 0) {
		process_t *proc = (process_t *)heap_pop(readyq);
		total_wait_time += current_time;
		return proc;
	} else {
		return NULL;
	}
}
process_t *ready_peek() {
	return readyq->length > 0 ? (process_t *)readyq->elements[0] : NULL;
}
void dump_readyq() {
	printf("[Q");
	if (readyq->length == 0) {
		printf(" <empty>]\n");
	} else {
		heapq_t heap[1];
		memcpy(heap, readyq, sizeof(heapq_t));
		while (heap->length > 0) {
			printf(" %c", ((process_t *)heap_pop(heap))->id);
		}
		printf("]\n");
	}
}
void add_to_readyq(process_t *proc, char *reason) {//printing 
	ready_push(proc, rr_add);
	printf("time %dms: Process %c %s added to ready queue ", current_time, proc->id, reason);
	dump_readyq();
}


void parseline( char* buffer, int size, process_t *processes, int *n);
process_t *parse(char *filename, process_t *processes, int *n);
void preempt_srt(process_t *new_proc);
void fcfs();
void srt();
void rr();


int main(int argc, char **argv){

	process_t processes[MAXSIZE]; /* Array of all processes */
	int n = 0; /* number of processes to simulate */

    if (argc != 3 && argc != 4) {
    	fprintf(stderr, "ERROR: Invalid arguments\n");
    	fprintf(stderr, "USAGE: %s <input-file> <stats-output-file> [<rr-add>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (argc == 4 && strcmp(argv[3], "BEGINNING") == 0) {
		rr_add = 1;
	} // else END

	struct stat file_info;
	lstat(argv[1], &file_info);
	if (!S_ISREG(file_info.st_mode)) { // add more
		fprintf(stderr, "ERROR: Invalid input file formate\n");
		return EXIT_FAILURE;
	}

	parse(argv[1], processes, &n); /* store processes from file in array */

	fcfs();

	srt();

	rr();

	return 0;
}


void parseline( char* buffer, int size, process_t *processes, int *n) {
	process_t proc;
	printf("%c\n", buffer[0]);
	if(buffer[0] == '#')
	{
		return;
	}
	int count = 1;
	char num_buf[10];
	proc.id = buffer[0];
	int i = 2;
	while(count < 5)
	{
		int j = 0;
		while(isalnum(buffer[i]))
		{
			num_buf[j] = buffer[i];
			j++;
			i++;
		}
		num_buf[j] = '\0';
		if(count == 1) {
			proc.init_arrival_time = atoi(num_buf);
			printf("arrival time: %d\n", proc.init_arrival_time);
		}
		else if(count == 2) {
			proc.cpu_burst_time = atoi(num_buf);
			printf( "cpu burst time: %d\n", proc.cpu_burst_time);
		}
		else if(count == 3) {
			proc.cpu_burst_count = atoi(num_buf);
			printf("num cpu bursts: %d\n", proc.cpu_burst_count);
		}
		else if(count == 4) {
			proc.io_burst_time = atoi(num_buf);
			printf("io burst time: %d\n", proc.io_burst_time);
		}
		count++;
		i++;
	}
	// now add process to processes array
	processes[*n] = proc;
	*n += 1;
}


process_t *parse(char *filename, process_t *processes, int *n) {
	FILE* file = fopen(filename, "r");
	if(file == NULL) {
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
	char *buffer = NULL;
	size_t size = 0;
	//buffer = (char*)malloc(50*sizeof(char));
	int bytes_read = getline(&buffer, &size, file);
	//printf("%s : bytes_read = %d\n", buffer, bytes_read);
	while(bytes_read != -1) {
		parseline(buffer, bytes_read, processes, n);
		bytes_read = getline(&buffer, &size, file);
	}
	if(bytes_read == -1) {
		free( buffer );
		fclose( file );
	}
	return processes;
}


int cmp_fcfs(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// compare cpu burst arrival time
	if (p1->time_slice->time < p2->time_slice->time)
		return -1;
	else if (p1->time_slice->time == p2->time_slice->time)
		return 0;
	else
		return 1;
}


int cmp_srt(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// compare remaining time
	if (p1->remaining_burst_time < p2->remaining_burst_time)
		return -1;
	if (p1->remaining_burst_time == p2->remaining_burst_time)
		return 0;
	else
		return 1;
}


int cmp_rr(void *_p1, void *_p2) {
	// compare arrival time... but we also need to divide bursts
	return 0;
}


// check if newly arrived process has less remaining time than current process
// switch out current process w/ new, put current in ready queue
// skips adding new_proc to ready queue
void preempt_srt(process_t *new_proc) {
	// copy new process first?
	process_t tmp = *current_process;
	heap_push(readyq, &tmp);
	current_process = new_proc;
}


void preempt_rr() {

}


void fcfs() {

}


void srt() {

}


void rr() {

}


void print_to_file() {
	// int fd = 
}