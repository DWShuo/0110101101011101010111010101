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

void parser( char* buffer, int size)
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
}

void print_usage(char *cmd) {
	printf("USAGE: %s <input-file> <stats-output-file> [<rr-add>]", cmd);
}

int main(int argc, char **argv){
    if (argc != 3 && argc != 4) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}
	FILE* file = fopen(argv[1], "r");
	if(file == NULL)
	{
		printf("uh-oh\n");
	}
	char *buffer = NULL;
	size_t size = 0;
	//buffer = (char*)malloc(50*sizeof(char));
	int bytes_read = getline(&buffer, &size, file);
	//printf("%s : bytes_read = %d\n", buffer, bytes_read);
	while(bytes_read != -1)
	{
		parser(buffer, bytes_read);
		bytes_read = getline(&buffer, &size, file);
	}
	if(bytes_read == -1)
	{
		free( buffer );
		fclose( file );
	}


return 0;
}
