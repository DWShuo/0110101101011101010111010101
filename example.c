#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heapq.h"

heapq_t eventq[1];

int cmp_event_time(void *_e1, void *_e2) {
	event_t *e1 = (event_t *)_e1;
	event_t *e2 = (event_t *)_e2;
	
}

heap_init(eventq, cmp_event_time);




