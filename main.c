#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heapq.h"

#define MAXLINE 100

//EVENT AND PROCESS STRUCTS
char *schedular_name[3] = {"FCFS", "SRT", "RR"}

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

//COMPARISON FUCNTIONS


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

//HEAP OPERATIONS


void print_usage(char *cmd) {
	printf("USAGE: %s <input-file> <stats-output-file> [<rr-add>]", cmd);
}

int main(int argc, char **argv){
    if (argc != 3 && argc != 4) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}


return 0;
}
