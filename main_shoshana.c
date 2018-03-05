// TO DO: make sure remaining_burst_time is updating properly

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
int t_cs = 6; // make sure this is 8 for submission
int t_slice = 80;
char msg[MAXLINE];

/* stats */
int total_cpu_burst_time; // will be divided by # of bursts
int total_wait_time; // will be divided by # of bursts
int total_turnaround_t; // will be divided by # of bursts
int total_num_cs;
int total_num_preempts;
int num_bursts;

int cmp_fcfs(void *_p1, void *_p2);
int cmp_srt(void *_p1, void *_p2);
void parseline( char* buffer, int size, process_t *processes, int *n);
process_t *parse(char *filename, process_t *processes, int *n);
void preempt_srt(int pre1, int pre2, process_t **cur_proc, int32_t t, int32_t *prev_t,
		heapq_t *readyq, int *terminated, int *switch_out);
void fcfs(process_t *process_t, int n, char *stat_string);
void srt(process_t *processes, int n, char *stat_string);
void rr(process_t *processes, int n, char *stat_string);
void clear_file(char *output_name);
void print_to_file(char *stat_string, char *output_name);
void reset_processes(process_t *processes, int n);


int main(int argc, char **argv){

	process_t processes[MAXSIZE]; /* Array of all processes */
	int n = 0; /* number of processes to simulate */
	char stat_string[300]; /* string to hold CPU stats */

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

	clear_file(argv[2]);

	fcfs(processes, n, stat_string);
	print_to_file(stat_string, argv[2]);
	reset_processes(processes, n);

	srt(processes, n, stat_string);
	print_to_file(stat_string, argv[2]);
	// reset_processes(processes, n);

	// rr(processes, n, stat_string);
	// write stat_string to file

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
			proc.remaining_burst_time = proc.cpu_burst_time;
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


/* compares processes based on remaining CPU burst time */
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


void create_stats_string(char *stat_string, char *title) {
	sprintf(stat_string, "%s\n"
		"-- average CPU burst time: %.2f ms\n"
		"-- average wait time: %.2f ms\n"
		"-- average turnaround time: %.2f ms\n"
		"-- total number of context switches: %d\n"
		"-- total number of preemptions: %d\n",
		title, (float)total_cpu_burst_time/num_bursts, (float)total_wait_time/num_bursts,
		(float)total_turnaround_t/num_bursts, total_num_cs, total_num_preempts);
	// printf("%s", stat_string);
}


void reset_processes(process_t *processes, int n) {
	for (int i = 0; i < n; ++i) {
		processes[i].next_arrival_time = processes[i].init_arrival_time;
		processes[i].cpu_bursts_remaining = processes[i].cpu_burst_count;
		// start time?
		// remaining burst time... not sure if I take care of this somewhere else
		processes[i].remaining_burst_time = processes[i].cpu_burst_time;
	}
	printf("\n");
}


/* initialize ready queue with processes w/ arrival time 0 */
void init_readyq(process_t *processes, int n, int32_t t, heapq_t *readyq, heapq_t *futureq) {
	for (int i = 0; i < n; ++i) {
		if (processes[i].init_arrival_time == t) {
			heap_push(readyq, processes+i);
			sprintf(msg, "Process %c arrived and added to ready queue", processes[i].id);
			print_event(t, msg, readyq);
		} else {
			heap_push(futureq, processes+i);
		}
	}
}


// should I be combining this with add_blocked_readyq? Only difference is message
/* add processes finished with I/O */
int add_blocked_readyq(heapq_t *readyq, heapq_t *blockedq, int32_t t, process_t *cur_proc) {
	if (blockedq->length > 0) {
		process_t arrived[MAXSIZE];
		process_t *next = peek(blockedq);
		int i = 0;
		while (blockedq->length > 0 && next->next_arrival_time <= t) {
			heap_push(readyq, heap_pop(blockedq));
			// if preemption... should say preempt
			// sprintf(msg, "Process %c completed I/O; added to ready queue", next->id);
			// print_event(t, msg, readyq);
			arrived[i] = *next;
			next = peek(blockedq);
			i += 1;
		}

		int n = i;
		for (i = 0; i < n; ++i) {
			// I think the comparison isn't quite right because remaining_burst_time should be updated
			if (cur_proc != NULL && arrived[i].remaining_burst_time < cur_proc->remaining_burst_time) {
				process_t *next = heap_pop(readyq); // hack-y and not good
				// printf("%d %d\n", arrived[i].remaining_burst_time, cur_proc->remaining_burst_time);
				sprintf(msg, "Process %c completed I/O and will preempt %c", arrived[i].id, cur_proc->id);
				print_event(t, msg, readyq);
				heap_push(readyq, next); // is this why it shows twice?
			}
			else {
				sprintf(msg, "Process %c completed I/O; added to ready queue", arrived[i].id);
				print_event(t, msg, readyq);
			}
		}
	}
	if (readyq->length > 0) {
		process_t *next = peek(readyq);
		return next->remaining_burst_time;
	} else {
		return 0;
	}
}

/* add new arrivals */
int add_new_readyq(heapq_t *readyq, heapq_t *futureq, int32_t t, process_t *cur_proc) {
	if (futureq->length > 0) {
		process_t arrived[MAXSIZE];
		process_t *next = peek(futureq);
		int i = 0;
		while (futureq->length > 0 && next->next_arrival_time <= t) {
			heap_push(readyq, heap_pop(futureq));
			// if preemption... should say preempt
			// sprintf(msg, "Process %c arrived and added to ready queue", next->id);
			// print_event(t, msg, readyq);
			arrived[i] = *next;
			next = peek(futureq);
			i += 1;
		}

		int n = i;
		for (i = 0; i < n; ++i) {
			// cur_proc->remaining_burst_time hasn't been fixed, needs to be updated
			if (cur_proc != NULL && arrived[i].remaining_burst_time < cur_proc->remaining_burst_time) {
				process_t *next = heap_pop(readyq); // hack-y and not good
				sprintf(msg, "Process %c arrived and will preempt %c", arrived[i].id, cur_proc->id);
				print_event(t, msg, readyq);
				heap_push(readyq, next); // is this why it shows twice?
			}
			else {
				sprintf(msg, "Process %c arrived and added to ready queue", arrived[i].id);
				print_event(t, msg, readyq);
			}
		}
	}
	if (readyq->length > 0) {
		process_t *next = peek(readyq);
		return next->remaining_burst_time;
	} else {
		return 0;
	}
}


/* update process that finished running so that you know when to run it next */
void update_process(process_t *cur_proc, int32_t t, heapq_t *blockedq, heapq_t *readyq, int *terminated) {
	total_cpu_burst_time += cur_proc->cpu_burst_time;
	// total_wait_time +=   // a little more complicated...
	total_turnaround_t += (t - cur_proc->next_arrival_time + t_cs/2);
	total_num_cs += 1;
	num_bursts += 1;
	
	if (cur_proc->cpu_bursts_remaining == 1) { // issue w/ SRT when preempted during last process and then comes back into cpu
		*terminated += 1;
		sprintf(msg, "Process %c terminated", cur_proc->id);
		print_event(t, msg, readyq);
	}
	else {
		// This might still hold in SRT
		cur_proc->next_arrival_time = t + t_cs/2 + cur_proc->io_burst_time;
		cur_proc->cpu_bursts_remaining -= 1;
		heap_push(blockedq, cur_proc);
		if (cur_proc->cpu_bursts_remaining > 1)
			sprintf(msg, "Process %c completed a CPU burst; %d bursts to go", cur_proc->id, cur_proc->cpu_bursts_remaining);
		else
			sprintf(msg, "Process %c completed a CPU burst; %d burst to go", cur_proc->id, cur_proc->cpu_bursts_remaining);
		print_event(t, msg, readyq);
		sprintf(msg, "Process %c switching out of CPU; will block on I/O until time %dms", cur_proc->id, cur_proc->next_arrival_time);
		print_event(t, msg, readyq);
	}
}


/* First Come First Serve */
void fcfs(process_t *processes, int n, char *stat_string) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	heap_init(readyq, cmp_fcfs);
	heap_init(blockedq, cmp_fcfs); // next arrival time signifies end of I/O time
	heap_init(futureq, cmp_fcfs); // look at future based on arrival time

	int32_t t = 0;
	int32_t prev_t = 0;
	int terminated = 0;
	int switch_out = 0;
	int switch_in = 1;

	total_cpu_burst_time = 0; // will be divided by # of bursts
	total_wait_time = 0; // will be divided by # of bursts
	total_turnaround_t = 0; // will be divided by # of bursts
	total_num_cs = 0;
	total_num_preempts = 0;
	num_bursts = 0;

	sprintf(msg, "Simulator started for FCFS");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	// end of switch out can go directly to switch in or nothing
	// end of switch goes directly to beginning of cpu burst
	// end of cpu burst goes directly switch out

	// change conditions to easy-to-read states?
	while (terminated < n) {
		/* ONE: CPU burst completion */

		/* check if current process exists and is finished */
		if (cur_proc != NULL && !switch_in && !switch_out &&
				(cur_proc->cpu_burst_time) == t - prev_t) {
			update_process(cur_proc, t, blockedq, readyq, &terminated);
			cur_proc = NULL;
			switch_out = 1;
			prev_t = t;
		}

		/* check if in switch out state and if finished */
		if (switch_out && (t - prev_t == t_cs/2)) { // when >= it terminates, may need to change order
			switch_out = 0;
			prev_t = t;
		}

		/* TWO: I/O burst completion */

		/* add to ready queue */
		add_blocked_readyq(readyq, blockedq, t, NULL);
		add_new_readyq(readyq, futureq, t, NULL);

		/* THREE: process arrival */

		/* start running next process in ready queue if no current process */
		if (readyq->length > 0 && cur_proc == NULL && !switch_out && !switch_in) {
			cur_proc = heap_pop(readyq);
			switch_in = 1;
			prev_t = t;
		}

		/* check if in switch in state and if finished */
		if (switch_in && (t - prev_t == t_cs/2)) {
			switch_in = 0;
			prev_t = t;
			sprintf(msg, "Process %c started using the CPU", cur_proc->id);
			print_event(t, msg, readyq);

		}

		t += 1;
	}

	t += (t_cs/2 - 1); // makes up for not finishing last switch out

	printf("time %dms: Simulator ended for FCFS\n", t);

	create_stats_string(stat_string, "Algorithm FCFS");
}


void update_process_preempt(process_t *cur_proc, int32_t t, int32_t prev_t, heapq_t *readyq) {
	total_cpu_burst_time += cur_proc->cpu_burst_time;
	// total_wait_time +=   // a little more complicated...
	total_turnaround_t += (t - cur_proc->next_arrival_time + t_cs/2);
	total_num_cs += 1;
	num_bursts += 1;

	cur_proc->next_arrival_time = t + t_cs/2 + cur_proc->io_burst_time;
	// cur_proc->remaining_burst_time = cur_proc->remaining_burst_time - (t - prev_t);
	heap_push(readyq, cur_proc);
}


// check if newly arrived process has less remaining time than current process
// switch out current process w/ new, put current in ready queue
// skips adding new_proc to ready queue
void preempt_srt(int pre1, int pre2, process_t **cur_proc, int32_t t, int32_t *prev_t,
		heapq_t *readyq, int *terminated, int *switch_out) {
	// save remaining time of current process that is being preempted
	// how will switch_in/switch out work? just set to switch out?
	if (*cur_proc != NULL && ((pre1 != 0  && pre1 < (*cur_proc)->remaining_burst_time) ||
			(pre2 != 0 && pre2 < (*cur_proc)->remaining_burst_time))) {

		total_num_preempts += 1;

		update_process_preempt(*cur_proc, t, *prev_t, readyq);

		*cur_proc = NULL;
		*switch_out = 1;
		*prev_t = t;
	}
}


/* Shortest Remaining Time */
void srt(process_t *processes, int n, char *stat_string) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	heap_init(readyq, cmp_srt);
	heap_init(blockedq, cmp_fcfs); // not sure about this yet
	heap_init(futureq, cmp_fcfs); // I think this should be sorted by arrival time

	int32_t t = 0;
	int32_t prev_t = 0;
	int terminated = 0;
	int switch_out = 0;
	int switch_in = 1;

	total_cpu_burst_time = 0; // will be divided by # of bursts
	total_wait_time = 0; // will be divided by # of bursts
	total_turnaround_t = 0; // will be divided by # of bursts
	total_num_cs = 0;
	total_num_preempts = 0;
	num_bursts = 0;

	sprintf(msg, "Simulator started for SRT");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	while (terminated < n) {
		/* ONE: CPU burst completion */

		/* check if current process exists and is finished */
		if (cur_proc != NULL && !switch_in && !switch_out &&
				// (cur_proc->remaining_burst_time) == t - prev_t) {
				(cur_proc->remaining_burst_time == 0)) {
			cur_proc->remaining_burst_time = 0;
			update_process(cur_proc, t, blockedq, readyq, &terminated);
			cur_proc->remaining_burst_time = cur_proc->cpu_burst_time;
			cur_proc = NULL;
			switch_out = 1;
			prev_t = t;
		}

		/* check if in switch out state and if finished */
		if (switch_out && (t - prev_t == t_cs/2)) {
			switch_out = 0;
			prev_t = t;
		}

		/* TWO: I/O burst completion */

		/* add to ready queue */
		// preemption should be checked in these functions
		int preempt1 = add_blocked_readyq(readyq, blockedq, t, cur_proc);
		int preempt2 = add_new_readyq(readyq, futureq, t, cur_proc);
		// issue is that next switch_out happens on next iteration of loop ???
		preempt_srt(preempt1, preempt2, &cur_proc, t, &prev_t, readyq,
			&terminated, &switch_out);

		/* THREE: process arrival */

		/* start running next process in ready queue if no current process */
		if (cur_proc == NULL && readyq->length > 0 && !switch_out && !switch_in) {
			cur_proc = heap_pop(readyq);
			switch_in = 1;
			prev_t = t;
		}

		/* check if in switch in state and if finished */
		if (switch_in && (t - prev_t == t_cs/2)) {
			switch_in = 0;
			prev_t = t;
			// issue here
			if (cur_proc->cpu_burst_time == cur_proc->remaining_burst_time)
				sprintf(msg, "Process %c started using the CPU", cur_proc->id);
			else
				sprintf(msg, "Process %c started using the CPU with %dms remaining", cur_proc->id, cur_proc->remaining_burst_time);
			print_event(t, msg, readyq);

		}

		if (cur_proc != NULL && !switch_in) {
			cur_proc->remaining_burst_time -= 1;
		}

		t += 1;
	}

	t += (t_cs/2 - 1); // makes up for not finishing last switch out

	printf("time %dms: Simulator ended for SRT\n", t);

	create_stats_string(stat_string, "Algorithm SRT");
}


/* Round Robin */
void rr(process_t *processes, int n, char *stat_string) {
	heapq_t readyq[1]; /* ready state */
	heapq_t futureq[1]; /* not yet arrived */
	heapq_t blockedq[1]; /* blocked state */
	process_t *cur_proc; /* running state */

	// I need to think about this...
	heap_init(readyq, cmp_srt);
	heap_init(blockedq, cmp_fcfs); // not sure about this yet
	heap_init(futureq, cmp_fcfs); // I think this should be sorted by arrival time

	int32_t t = 0;
	// int32_t prev_t = 0;
	// int terminated = 0;
	// int switch_out = 0;
	// int switch_in = 1;

	total_cpu_burst_time = 0; // will be divided by # of bursts
	total_wait_time = 0; // will be divided by # of bursts
	total_turnaround_t = 0; // will be divided by # of bursts
	total_num_cs = 0;
	total_num_preempts = 0;
	num_bursts = 0;

	sprintf(msg, "Starting simulation for Round Robin");
	print_event(t, msg, readyq);

	init_readyq(processes, n, t, readyq, futureq);
	assert(readyq->length > 0);

	if (readyq->length > 0) {
		cur_proc = heap_pop(readyq);
	}

	printf("time %dms: Simulator ended for RR\n", t);

	// create_stats_string(stat_string, "Algorithm RR");
}


void clear_file(char *output_name) {
	FILE *file = fopen(output_name, "w"); /* clear output file */
	if (file == NULL) {
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
	fclose(file);
}

/* print stats to output file */
void print_to_file(char *stat_string, char *output_name) {
	FILE *file = fopen(output_name, "a");
	if (file == NULL) {
		fprintf(stderr, "ERROR: fopen failed\n");
		exit(EXIT_FAILURE);
	}
	fprintf(file, "%s\n", stat_string);

	fclose(file);
}
