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
