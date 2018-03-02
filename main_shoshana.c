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

struct _event_t;
typedef struct {
	char id;    			 //   ]
	int32_t init_arrival_time;//  ] same thing as enqueue time?
	int32_t cpu_burst_time;  //   ]-- from the input file
	int32_t cpu_burst_count; //   ]
	int32_t io_burst_time;   //   ] 
	int32_t next_arrival_time;
	int32_t cpu_bursts_remaining;
	// for SRT/RR
	int32_t start_time;  // latest time the process entered the running state
	int32_t remaining_burst_time;
} process_t;

/* simulator global variables */
int rr_add = 0;
int t_cs = 8;
int t_slice = 80;
char msg[MAXLINE];

int cmp_fcfs(void *_p1, void *_p2);
int cmp_srt(void *_p1, void *_p2);
void parseline( char* buffer, int size, process_t *processes, int *n);
process_t *parse(char *filename, process_t *processes, int *n);
void preempt_srt(heapq_t *readyq, process_t *cur_proc, process_t *new_proc);
void fcfs(process_t *process_t, int *n);
void srt(process_t *processes, int *n);
void rr(process_t *processes, int *n);
void print_to_file();


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
		fprintf(stderr, "ERROR: Invalid input file format\n");
		return EXIT_FAILURE;
	}

	parse(argv[1], processes, &n); /* store processes from file in array */

	fcfs(processes, &n);

	// reset first
	srt(processes, &n);

	// reset first
	rr(processes, &n);

	print_to_file();

	return 0;
}


/* parses each line of file and stores as a process */
void parseline(char* buffer, int size, process_t *processes, int *n) {
	process_t proc;
	if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n') {
		return;
	}
	int count = 1;
	char num_buf[10];
	proc.id = buffer[0];
	int i = 2;
	while(count < 5) {
		int j = 0;
		while(isalnum(buffer[i])) {
			num_buf[j] = buffer[i];
			j++;
			i++;
		}
		num_buf[j] = '\0';
		if(count == 1) {
			proc.init_arrival_time = atoi(num_buf);
			proc.next_arrival_time = proc.init_arrival_time;
		}
		else if(count == 2) {
			proc.cpu_burst_time = atoi(num_buf);
		}
		else if(count == 3) {
			proc.cpu_burst_count = atoi(num_buf);
			proc.cpu_bursts_remaining = proc.cpu_burst_count;
		}
		else if(count == 4) {
			proc.io_burst_time = atoi(num_buf);
		}
		count++;
		i++;
	}

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

	char *buffer = NULL;
	size_t size = 0;
	int bytes_read = getline(&buffer, &size, file);
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


/* compares processes based on arrival time */
// ties should be broken by process ID
int cmp_fcfs(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// compare cpu burst arrival time
	if (p1->next_arrival_time < p2->next_arrival_time ||
			(p1->next_arrival_time == p2->next_arrival_time && p1->id < p2->id))
		return -1;
	else if (p1->next_arrival_time == p2->next_arrival_time &&
			p1->id == p2->id)
		return 0;
	else
		return 1;
}


/* compares processes based on remaining CPU burse time */
int cmp_srt(void *_p1, void *_p2) {
	process_t *p1 = (process_t *)_p1;
	process_t *p2 = (process_t *)_p2;
	// compare remaining time
	if (p1->remaining_burst_time < p2->remaining_burst_time ||
			(p1->remaining_burst_time == p2->remaining_burst_time
				&& p1->id < p2->id))
		return -1;
	if (p1->remaining_burst_time == p2->remaining_burst_time &&
			p1->id == p2->id)
		return 0;
	else
		return 1;
}


// check if newly arrived process has less remaining time than current process
// switch out current process w/ new, put current in ready queue
// skips adding new_proc to ready queue
void preempt_srt(heapq_t *readyq, process_t *cur_proc, process_t *new_proc) {
	// copy new process first?
	process_t tmp = *cur_proc;
	heap_push(readyq, &tmp);
	cur_proc = new_proc;
}


void preempt_rr() {

}


/* output important events */
void print_event(int32_t t, char *msg, heapq_t *readyq) {
	if (readyq->length > 0) {
		char ids[52];
		heapq_t tmp = *readyq;
		int i = 0;
		while (tmp.length > 0 && i < 52) {
			ids[i] = ((process_t*)heap_pop(&tmp))->id;
			ids[i+1] = ' ';
			i += 2;
		}
		ids[i-1] = '\0';
		printf("time %dms: %s [Q %s]\n", t, msg, ids);
	} else {
		char *ids = "<empty>";
		printf("time %dms: %s [Q %s]\n", t, msg, ids);
	}
}


/* initialize ready queue with processes w/ arrival time 0 */
void init_readyq(process_t *processes, int *n, int32_t t, heapq_t *readyq, heapq_t *futureq) {
	for (int i = 0; i < *n; ++i) {
		if (processes[i].init_arrival_time == t) {
			heap_push(readyq, processes+i);
			sprintf(msg, "Process %c arrives and added to ready queue", processes[i].id);
			print_event(t, msg, readyq);
		} else {
			heap_push(futureq, processes+i);
		}
	}
}


/* add processes finished with I/O */
void add_blocked_readyq(heapq_t *readyq, heapq_t *blockedq, int32_t t) {
	if (blockedq->length > 0) {
		process_t *next = peek(blockedq);
		while (blockedq->length > 0 && next->next_arrival_time <= t) {
			heap_push(readyq, heap_pop(blockedq));
			sprintf(msg, "Process %c finishes performing I/O", next->id);
			print_event(t, msg, readyq);
			next = peek(blockedq);
		}
	}
}


/* add new arrivals */
void add_new_readyq(heapq_t *readyq, heapq_t *futureq, int32_t t) {
	if (futureq->length > 0) {
		process_t *next = peek(futureq);
		while (futureq->length > 0 && next->next_arrival_time <= t) {
			heap_push(readyq, heap_pop(futureq));
			sprintf(msg, "Process %c arrives and added to ready queue", next->id);
			print_event(t, msg, readyq);
			next = peek(futureq);
		}
	}
}


/* update process that finished running so that you know when to run it next */
void update_process(process_t *cur_proc, int32_t t, heapq_t *blockedq, heapq_t *readyq, int *terminated) {
	if (cur_proc->cpu_bursts_remaining == 1) {
		// terminate
		*terminated += 1;
		sprintf(msg, "Process %c terminates by finishing its last CPU burst", cur_proc->id);
		print_event(t, msg, readyq);
	}
	else {
		printf("%c %d\n", cur_proc->id, cur_proc->cpu_bursts_remaining);
		cur_proc->next_arrival_time = t + t_cs/2 + cur_proc->io_burst_time; // + t_cs/2?
		cur_proc->cpu_bursts_remaining -= 1;
		heap_push(blockedq, cur_proc);
		sprintf(msg, "Process %c finishes using CPU", cur_proc->id);
		print_event(t, msg, readyq);
		sprintf(msg, "Process %c starts performing I/O", cur_proc->id);
		print_event(t, msg, readyq);
	}
}


/* First Come First Serve */
void fcfs(process_t *processes, int *n) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	heap_init(readyq, cmp_fcfs);
	heap_init(blockedq, cmp_fcfs); // except based on something else... I/O time
	heap_init(futureq, cmp_fcfs); // look at future based on arrival time

	int32_t t = 0;
	int32_t prev_t = 0;
	int terminated = 0;
	int switch_out = 0;
	int switch_in = 1;

	sprintf(msg, "Starting simulation for FCFS");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	while (terminated < *n) {
		/* check if in switch out state and if finished */
		if (switch_out && (t - prev_t == t_cs/2)) {
			// should I move some finish cpu stuff up here?
			switch_out = 0;
			switch_in = 1;
			prev_t = t;

		}
		/* check if in switch in state and if finished */
		if (switch_in && (t - prev_t == t_cs/2)) {
			switch_in = 0;
			prev_t = t;
			sprintf(msg, "Process %c starts using the CPU", cur_proc->id);
			print_event(t, msg, readyq);
		}
		/* check if current process exists and is finished */
		if (!switch_in && !switch_out && cur_proc != NULL &&
				(cur_proc->cpu_burst_time) == t - prev_t) {
			update_process(cur_proc, t, blockedq, readyq, &terminated);
			cur_proc = NULL;
			switch_out = 1;
		}
		/* start running next process in ready queue if no current process */
		if (readyq->length > 0 && cur_proc == NULL) {
			cur_proc = heap_pop(readyq);
			prev_t = t;
		}

		/* add to ready queue */
		add_blocked_readyq(readyq, blockedq, t);
		add_new_readyq(readyq, futureq, t);

		t += 1;
	}

	printf("time %dms: Simulator ended for FCFS\n", t);
}


/* Shortest Remaining Time */
void srt(process_t *processes, int *n) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	heap_init(readyq, cmp_srt);
	heap_init(blockedq, cmp_srt);
	heap_init(futureq, cmp_srt);

	int32_t t = 0;
	int32_t prev_t = 0;
	int terminated = 0;
	int switch_out = 0;
	int switch_in = 1;

	sprintf(msg, "Starting simulation for SRT");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	while (terminated < *n) {
		terminated += 1; // TO DO: implement
		/* add to ready queue */
		add_blocked_readyq(readyq, blockedq, t);
		add_new_readyq(readyq, futureq, t);
	}

	printf("time %dms: Simulator ended for SRT\n", t);
}


/* Round Robin */
void rr(process_t *processes, int *n) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	int32_t t = 0;
	int32_t prev_t = 0;
	int terminated = 0;
	int switch_out = 0;
	int switch_in = 1;

	sprintf(msg, "Starting simulation for Round Robin");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	printf("time %dms: Simulator ended for RR\n", t);
}

void avg_wait_time(process_t processes) {

}

void avg_turnaround(process_t processes) {

}


/* print stats to output file */
void print_to_file() {
	FILE *file = fopen("output.txt", "w");
	if (file == NULL) {
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
	fprintf(file, "Algorithm FCFS\n");
	fprintf(file, "Algorithm SRT\n");
	fprintf(file, "Algorithm RR\n");

	fclose(file);
}
