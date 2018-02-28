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

//EVENT AND PROCESS STRUCTS
char *schedular_name[3] = {"FCFS", "SRT", "RR"};

typedef enum{
    FCFS = 0,
    SRT = 1,
    RR = 2
} scheduler_t;

struct _event_t;
typedef struct {
	char name[MAXNAME+1];    //   ]
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

//TODO: implement compare functions
//COMPARE FUNCTIONS


//goes
//here


//COMPARE FUNCTION

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
			printf(" %s", ((process_t *)heap_pop(heap))->name);
		}
		printf("]\n");
	}
}
void add_to_readyq(process_t *proc, char *reason) {//printing 
	ready_push(proc, rr_add);
	printf("time %dms: Process %s %s added to ready queue ", current_time, proc->name, reason);
	dump_readyq();
}

//TODO: implement preempt
int preempt(process_t *proc, scheduler_t scheduler, char *reason) {
	if (current_process == NULL) {
		return 0;
	}
	//......
	
	//........
	
}   
//SIM
//TODO: 420 blaze and code
void run_simulator(scheduler_t scheduler, char *stats) {
	current_process = NULL;
	current_time = 0;
	
	// clear stats counters
	total_burst_time = 0;
	burst_count = 0;
	total_wait_time = 0;
	wait_count = 0;
	total_turnaround_time = 0;
	turnaround_count = 0;
	context_switches = 0;
	preemptions = 0;
	
	heap_init(readyq, scheduler == SRT ? cmp_remaining_burst_time : cmp_enqueue_time);
	printf("time 0ms: Simulator started for %s ", scheduler_name[scheduler]);
	dump_readyq();
	int prev_time = event_peek()->time;
	while (eventq->length > 0) {
		event_t *event = event_pop();
		if (event->cancelled) {
			free(event);
			continue;
		}
		process_t *proc = event->process;
		current_time = event->time;
		int type = event->type;
		free(event);
		
		int preempted = 0;
		switch (type) {
			case EVENT_PROCESS_ARRIVAL:
			case EVENT_IO_FINISHED:
			total_burst_time += proc->cpu_burst_time;
				burst_count++;
				total_turnaround_time -= current_time;
				proc->remaining_burst_time = proc->cpu_burst_time;
				if (scheduler == SRT) {
					preempted = preempt(proc, scheduler, type == EVENT_PROCESS_ARRIVAL ? "arrived" : "completed I/O");
				}
				if (!preempted) {
					add_to_readyq(proc, type == EVENT_PROCESS_ARRIVAL ? "arrived and" : "completed I/O;");
				}
				break;
			case EVENT_CONTEXT_SWITCH_IN:
			    break;
			case EVENT_TIME_SLICE:
			    break;
			case EVENT_CONTEXT_SWITCH_OUT:
			    break;
			default:
				break;
		}
	
}       

void parseline( char* buffer, int size);
void parse(char *filename);

int main(int argc, char **argv){

    if (argc != 3 && argc != 4) {
    	fprintf(stderr, "ERROR: Invalid arguments\n");
    	fprintf(stderr, "USAGE: %s <input-file> <stats-output-file> [<rr-add>]\n", argv[0]);
		return EXIT_FAILURE;
	}

	struct stat file_info;
	lstat(argv[1], &file_info);
	if (!S_ISREG(file_info.st_mode)) { // add more
		fprintf(stderr, "ERROR: Invalid input file formate\n");
		return EXIT_FAILURE;
	}
	parse(argv[1]);
	
	// clear stats file
	fclose(fopen(argv[2], "w"));
	int i;
	for (i = 0; i < 3; i++) {
		if (!read_file(argv[1])) {
			return EXIT_FAILURE;
		}
		if (i == RR && argc == 4) {
			rr_add = strcmp(argv[3], "BEGINNING") == 0;
		} else {
			rr_add = 0;
		}
		run_simulator(i, argv[2]);
		if (i < 2) {
			putchar('\n');
		}
	}
	
	return 0;
}
//TODO: unit test to see if object created are good
void parseline( char* buffer, int size)
{
	printf("%c\n", buffer[0]);
	if(buffer[0] == '#')
	{
		return;
	}
	int count = 1;
	char proc_id;
	int arrival_time;
	int burst_time;
	int num_bursts;
	int io_time;
	char num_buf[10];
	proc_id = buffer[0];
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
		if(count == 1)
		{
			arrival_time = atoi(num_buf);
			printf("%d\n", arrival_time);
		}
		else if(count == 2)
		{
			burst_time = atoi(num_buf);
			printf( "%d\n", burst_time);
		}
		else if(count == 3)
		{
			num_bursts = atoi(num_buf);
			printf("%d\n", num_bursts);
		}
		else if(count == 4)
		{
			io_time = atoi(num_buf);
			printf("%d\n", io_time);
		}
		count++;
		i++;
	}
	process_t *proc = (process_t *)calloc(sizeof(process_t), 1);
	strcpy(proc->name, proc_id);
	proc->cpu_burst_time = burst_time;
    proc->cpu_burst_count = num_bursts;
	proc->io_burst_time = io_time;
	event_t *event = create_event(EVENT_PROCESS_ARRIVAL, arrival_time, proc);
	event_push(event);
}

void parse(char *filename) {
	FILE* file = fopen(filename, "r");
	if(file == NULL)
	{
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
    heap_init(eventq, cmp_event_time);
	char *buffer = NULL;
	size_t size = 0;
	//buffer = (char*)malloc(50*sizeof(char));
	int bytes_read = getline(&buffer, &size, file);
	//printf("%s : bytes_read = %d\n", buffer, bytes_read);
	while(bytes_read != -1)
	{
		parseline(buffer, bytes_read);
		bytes_read = getline(&buffer, &size, file);
	}
	if(bytes_read == -1)
	{
		free( buffer );
		fclose( file );
	}
}
