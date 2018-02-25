#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heapq.h"

// statistics
int32_t total_burst_time;
int32_t burst_count;
int32_t total_wait_time;
int32_t wait_count;
int32_t total_turnaround_time;
int32_t turnaround_count;
int32_t context_switches;
int32_t preemptions;

void print_usage(char *cmd) {
	printf("USAGE: %s <input-file> <stats-output-file> [<rr-add>]", cmd);
}

int main(int argc, char **argv){
    if (argc != 3 && argc != 4) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	heapq_t h;
	int (*cmp_ptr)(void*, void*) = &cmp;
	heap_init(&h, cmp_ptr);

	int item = 4;
	heap_push(&h, &item);
	int *popped = heap_pop(&h);
	printf("%d\n", *popped);

	return 0;
}
