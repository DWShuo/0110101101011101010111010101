#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
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
	// 
	int32_t next_arrival_time;
	int32_t cpu_bursts_remaining;
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
int cmp_event_time(void *_p1, void *_p2);


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
void fcfs(process_t *process_t, int *n);
void srt(process_t *processes, int *n);
void rr(process_t *processes, int *n);


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

	fcfs(processes, &n);

	return 0;
}


/* parses each line of file and stores as a process */
void parseline(char* buffer, int size, process_t *processes, int *n) {
	process_t proc;
	printf("%c\n", buffer[0]);
	if(buffer[0] == '#') {
		return;
	}
	int count = 1;
	char num_buf[10];
	proc.id = buffer[0];
	int i = 2;
	while(count < 5) {
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
			proc.next_arrival_time = proc.init_arrival_time;
		}
		else if(count == 2)
		{
			proc.cpu_burst_time = atoi(num_buf);
		}
		else if(count == 3)
		{
			proc.cpu_burst_count = atoi(num_buf);
			proc.cpu_bursts_remaining = proc.cpu_burst_count;
		}
		else if(count == 4)
		{
			proc.io_burst_time = atoi(num_buf);
		}
		count++;
		i++;
	}
	event_t *event = create_event(EVENT_PROCESS_ARRIVAL, proc.init_arrival_time, &proc);
	event_push(event);

	processes[*n] = proc;
	*n += 1;
}


/* parses input file and stores in processes array */
process_t *parse(char *filename, process_t *processes, int *n) {
	FILE* file = fopen(filename, "r");
	if(file == NULL)
	{
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
    heap_init(eventq, cmp_event_time);
	char *buffer = NULL;
	size_t size = 0;
	int bytes_read = getline(&buffer, &size, file);
	while(bytes_read != -1)
	{
		parseline(buffer, bytes_read, processes, n);
		bytes_read = getline(&buffer, &size, file);
	}
	if(bytes_read == -1)
	{
		free( buffer );
		fclose( file );
	}

	return processes;
}


int cmp_fcfs(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// compare cpu burst arrival time
	if (p1->next_arrival_time < p2->next_arrival_time)
		return -1;
	else if (p1->next_arrival_time == p2->next_arrival_time)
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

int cmp_event_time(void *_p1, void *_p2) {
	event_t *p1 = (event_t *)_p1;
	event_t *p2 = (event_t *)_p2;
	// needs to be implemented
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


void init_readyq(process_t *processes, int *n, int32_t t, heapq_t *futureq) {
	printf("n: %d\n", *n);
	for (int i = 0; i < *n; ++i) {
		if (processes[i].init_arrival_time == t) { // shouldn't be init_arrival_time
			heap_push(readyq, processes+i);
		} else {
			heap_push(futureq, processes+i);
		}
	}
}


/* add processes finished with I/O */
void add_blocked_readyq(heapq_t *blockedq, int32_t t) {
	// check blocked queue and things that haven't been added yet
	// peek and see if start time <= t
	if (blockedq->length > 0) {
		process_t *next = peek(blockedq);
		while (blockedq->length > 0 && next->next_arrival_time <= t) {
			printf("id: %c   next_arrival_time: %d\n", next->id, next->next_arrival_time);
			printf("next blocked: %c\n", next->id);
			heap_push(readyq, heap_pop(blockedq));
			printf("time %dms: Process %c finishes performing I/O [Q stuff]\n", t, next->id);
			next = peek(blockedq);
		}
	}
}


/* add new arrivals */
void add_new_readyq(heapq_t *futureq, int32_t t) {
	// peek and see if start time <= t
	if (futureq->length > 0) {
		process_t *next = peek(futureq);
		while (futureq->length > 0 && next->next_arrival_time <= t) {
			printf("id: %c   next_arrival_time: %d\n", next->id, next->next_arrival_time);
			printf("next new: %c\n", next->id);
			heap_push(readyq, heap_pop(futureq));
			next = peek(futureq);
		}
	}
}


/* update process that finished running so that you know when to run it next */
void update_process(process_t *cur_proc, int32_t t, heapq_t *blockedq) {
	if (cur_proc->cpu_bursts_remaining == 1) {
		// terminate
		printf("time %dms: Process %c terminates by finishing its last CPU burst [Q stuff]\n", t, cur_proc->id);
	}
	else {
		printf("%c %d\n", cur_proc->id, cur_proc->cpu_bursts_remaining);
		cur_proc->next_arrival_time = t + cur_proc->io_burst_time; // + t_cs/2?
		cur_proc->cpu_bursts_remaining -= 1;
		heap_push(blockedq, cur_proc);
		printf("time %dms: Process %c finishes using CPU [Q stuff]\n", t, cur_proc->id);
		printf("time %dms: Process %c starts performing I/O [Q stuff]\n", t, cur_proc->id);
	}
}


void fcfs(process_t *processes, int *n) {
	heapq_t futureq[1];
	heapq_t blockedq[1];
	process_t *cur_proc;

	heap_init(readyq, cmp_fcfs);
	heap_init(blockedq, cmp_fcfs); // except based on something else... I/O time
	heap_init(futureq, cmp_fcfs); // look at future based on arrival time

	int32_t t = 0;
	init_readyq(processes, n, t, futureq);
	assert(readyq->length > 0);
	
	// while (readyq->length > 0) { // I think I need to check every second...
	// 	cur_proc = heap_pop(readyq);
	// 	printf("time %dms: Process %c starts using the CPU [Q stuff]\n", t, cur_proc->id);

	// 	t += t_cs; // probably shouldn't add time at the end
	// 	t += cur_proc->cpu_burst_time;
	// 	printf("time %dms: Process %c finishes using CPU [Q stuff]\n", t, cur_proc->id);
	// 	// check if I can take from blocked
	// 	add_blocked_readyq(blockedq, t);
	// 	add_new_readyq(futureq, t);

	// 	// first see if cur_proc is terminated or not
	// 	update_process(cur_proc, t);
	// }

	int32_t prev_t = 0;
	cur_proc = heap_pop(readyq);
	printf("time %dms: Process %c starts using the CPU [Q stuff]\n", t, cur_proc->id);
	// while (readyq->length > 0) { // the issue is that when b is popped there is nothing
	int i = 0;
	while (i < 100) {
		// check if current process is finished
			// if it is, update process and pop next process
		if (cur_proc->cpu_burst_time == t - prev_t) {
			update_process(cur_proc, t, blockedq);
			cur_proc = heap_pop(readyq);
			printf("time %dms: Process %c starts using the CPU [Q stuff]\n", t, cur_proc->id);
			prev_t = t;
			i += 1;
		}

		// add to ready queue
		add_blocked_readyq(blockedq, t);
		add_new_readyq(futureq, t);
		// printf("readyq->length: %d\n", readyq->length);
		// check I/O
		t += 1;
	}

	printf("time %dms: Simulator ended for FCFS\n", t);
}


void srt(process_t *processes, int *n) {

}


void rr(process_t *processes, int *n) {

}


void print_to_file() {
	// int fd = 
}
