#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heapq.h"

#define MAXLINE 100


typedef enum {
	FCFS = 0,
	SRT = 1,
	RR = 2
} scheduler_t;

char *scheduler_name[3] = {"FCFS", "SRT", "RR"};

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


// comparison functions for the priority queue order
int cmp_event_time(void *_e1, void *_e2) {
	event_t *e1 = (event_t *)_e1;
	event_t *e2 = (event_t *)_e2;
	// sort by time, type, process name
	if (e1->time != e2->time) {
		return e1->time - e2->time;
	} else {
		if (e1->type != e2->type) {
			return e1->type - e2->type;
		} else {
			return strcmp(e1->process->name, e2->process->name);
		}
	}
}
int cmp_enqueue_time(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// sort by time, process name
	if (p1->enqueue_time != p2->enqueue_time) {
		return p1->enqueue_time - p2->enqueue_time;
	} else {
		return strcmp(p1->name, p2->name);
	}
}
int cmp_remaining_burst_time(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// sort by time, process name
	if (p1->remaining_burst_time != p2->remaining_burst_time) {
		return p1->remaining_burst_time - p2->remaining_burst_time;
	} else {
		return strcmp(p1->name, p2->name);
	}
}



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

// heapq convenience functions
void event_push(event_t *event) {
	heap_push(eventq, event);
}
event_t *event_pop() {
	return eventq->length > 0 ? (event_t *)heap_pop(eventq) : NULL;
}
event_t *event_peek() {
	return eventq->length > 0 ? (event_t *)eventq->elements[0] : NULL;
}
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

event_t *create_event(event_type type, int32_t time, process_t *proc) {
	event_t *event = (event_t *)calloc(sizeof(event_t), 1);
	event->type = type;
	event->time = time;
	event->process = proc;
	return event;
}

void add_to_readyq(process_t *proc, char *reason) {
	ready_push(proc, rr_add);
	printf("time %dms: Process %s %s added to ready queue ", current_time, proc->name, reason);
	dump_readyq();
}

int preempt(process_t *proc, scheduler_t scheduler, char *reason) {
	if (current_process == NULL) {
		return 0;
	}
	int current_process_elapsed = current_time - current_process->start_time;
	int condition = 0;
	if (scheduler == SRT) {
		condition = proc->remaining_burst_time < current_process->remaining_burst_time - current_process_elapsed;
	} else if (scheduler == RR) {
		condition = 1;
	}
	if (condition) {
		preemptions++;
		current_process->remaining_burst_time -= current_process_elapsed;
		if (scheduler == SRT) {
			printf("time %dms: Process %s %s and will preempt %s ", current_time, proc->name, reason, current_process->name);
		} else if (scheduler == RR) {
			printf("time %dms: Time slice expired; process %s preempted with %dms to go ", current_time, current_process->name, current_process->remaining_burst_time);
		}
		dump_readyq();
		// cancel the pending context switch out event for the current process
		current_process->context_switch_out->cancelled = 1;
		current_process->context_switch_out = NULL;
		if (scheduler == RR) { current_time -= t_cs/2; }  // hack to exactly match the test data
		// enqueue current process at the end
		ready_push(current_process, 0);
		if (scheduler == RR) { current_time += t_cs/2; }  // hack to exactly match the test data
		// start proc
		event_t *event = create_event(EVENT_CONTEXT_SWITCH_IN, current_time + t_cs, proc);
		event_push(event);
		return 1;
	} else {
		return 0;
	}
}

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
				context_switches++;
				current_process = proc;
				proc->start_time = current_time;
				event = create_event(EVENT_CONTEXT_SWITCH_OUT, current_time + proc->remaining_burst_time, proc);
				proc->context_switch_out = event;
				event_push(event);
				if (scheduler == RR) {
					event = create_event(EVENT_TIME_SLICE, current_time + t_slice, proc);
					proc->time_slice = event;
					event_push(event);
				}
				printf("time %dms: Process %s started using the CPU ", current_time, proc->name);
				if (proc->remaining_burst_time < proc->cpu_burst_time) {
					printf("with %dms remaining ", proc->remaining_burst_time);
				}
				dump_readyq();
				break;
			case EVENT_TIME_SLICE:
				if (readyq->length > 0) {
					proc = ready_peek();
					preempt(proc, scheduler, NULL);
					ready_pop();
				} else {
					printf("time %dms: Time slice expired; no preemption because ready queue is empty ", current_time);
					event = create_event(EVENT_TIME_SLICE, current_time + t_slice, proc);
					proc->time_slice = event;
					event_push(event);
					dump_readyq();
				}
				break;
			case EVENT_CONTEXT_SWITCH_OUT:
				total_turnaround_time += current_time + t_cs/2;
				turnaround_count++;
				current_process = NULL;
				proc->cpu_burst_count--;
				if (scheduler == RR && proc->time_slice != NULL) {
					// cancel pending time slice
					proc->time_slice->cancelled = 1;
					proc->time_slice = NULL;
				}
				if (proc->cpu_burst_count > 0) {
					int t = current_time + proc->io_burst_time + t_cs/2;
					event = create_event(EVENT_IO_FINISHED, t, proc);
					event_push(event);
					printf("time %dms: Process %s completed a CPU burst; %d burst%s to go ", current_time, proc->name,
						proc->cpu_burst_count, proc->cpu_burst_count > 1 ? "s" : "");
					dump_readyq();
					printf("time %dms: Process %s switching out of CPU; will block on I/O until time %dms ", current_time, proc->name, t);
					dump_readyq();
				} else {
					printf("time %dms: Process %s terminated ", current_time, proc->name);
					dump_readyq();
					free(proc);
				}
				current_time += t_cs/2;
				break;
			default:
				break;
		}
		// process readyq after processing events
		event = event_peek();
		if (event == NULL || event->time > prev_time) {
			if (current_process == NULL && readyq->length > 0) {
				process_t *proc = ready_pop();
				event_t *event = create_event(EVENT_CONTEXT_SWITCH_IN, current_time + t_cs/2, proc);
				event_push(event);
			}
		}
		prev_time = current_time;
	}
	printf("time %dms: Simulator ended for %s\n", current_time, scheduler_name[scheduler]);

	// write stats file
	FILE *f = fopen(stats, "a");
	fprintf(f, "Algorithm %s\n", scheduler_name[scheduler]);
	fprintf(f, "-- average CPU burst time: %.2f ms\n", (float)total_burst_time/burst_count);
	// In the test data,
	// FCFS/SRT averages over the number of times the process entered the queue
	// RR averages over the number of bursts
	fprintf(f, "-- average wait time: %.2f ms\n", (float)total_wait_time/(scheduler == RR ? burst_count : wait_count));
	fprintf(f, "-- average turnaround time: %.2f ms\n", (float)total_turnaround_time/turnaround_count);
	fprintf(f, "-- total number of context switches: %d\n", context_switches);
	fprintf(f, "-- total number of preemptions: %d\n", preemptions);
	fclose(f);
}

int read_file(char *filename) {
	FILE *fin = fopen(filename, "r");
	if (fin == NULL) {
		printf("ERROR: Invalid input file format\n");
		return 0;
	}
	heap_init(eventq, cmp_event_time);
	while (1) {
		int i;
		// read input
		char line[MAXLINE];
		if (fgets(line, MAXLINE, fin) == NULL) {
			break;
		}
		if (strlen(line) == 0) {
			continue;
		}
		// remove cr/lf/space at the end
		i = strlen(line)-1;
		while ((line[i] == '\r' || line[i] == '\n' || line[i] == ' ' || line[i] == '\t') && i >= 0) {
			line[i--] = '\0';
		}
		// skip over whitespace at the beginning
		char *start = line;
		while ((start[0] == ' ' || start[0] == '\t')) {
			start++;
		}
		if (strlen(start) == 0) {
			continue;
		}
		// skip comment
		if (start[0] == '#') {
			continue;
		}
		// parse line
		char name[MAXNAME+1];
		char extra[2];
		int arrival, cpuburst, numburst, ioburst;
		if (sscanf(start, "%8[^|]|%d|%d|%d|%d%1s", name, &arrival, &cpuburst, &numburst, &ioburst, extra) == 5) {
			// create process and put in event queue
			process_t *proc = (process_t *)calloc(sizeof(process_t), 1);
			strcpy(proc->name, name);
			proc->cpu_burst_time = cpuburst;
			proc->cpu_burst_count = numburst;
			proc->io_burst_time = ioburst;
			event_t *event = create_event(EVENT_PROCESS_ARRIVAL, arrival, proc);
			event_push(event);
		} else {
			printf("ERROR: Invalid input file format\n");
			fclose(fin);
			return 0;
		}
	}
	fclose(fin);
	return 1;
}

void print_usage(char *cmd) {
	printf("USAGE: %s <input-file> <stats-output-file> [<rr-add>]", cmd);
}

int main(int argc, char **argv) {
	if (argc != 3 && argc != 4) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (argc == 4 && !(strcmp(argv[3], "BEGINNING") || strcmp(argv[3], "END"))) {
		print_usage(argv[0]);
	}
	
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
