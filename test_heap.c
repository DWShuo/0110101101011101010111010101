#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heapq.h"

int cmp(int32_t *p1, int32_t *p2) {
	if (*p1 < *p2)
		return -1;
	if (*p1 == *p2)
		return 0;
	else
		return 1;
}

void test_int() {
	heapq_t h;
	// int (*cmp_ptr)(void*, void*) = &cmp;
	int32_t popped;
	heap_init(&h, &cmp);

	int item = 4;
	heap_push(&h, &item);
	popped = *(int32_t*)heap_pop(&h);
	printf("%d\n", popped);

	int items[10] = {1, 2, 4, 7, 8, 3, 10, 0, 6, 5};
	for (int i = 0; i < 10; ++i) {
		heap_push(&h, items+i);
	}

	for (int i = 0; i < 10; ++i) {
		popped = *(int32_t*)heap_pop(&h);
		printf("%d\n", popped);
	}
}

void test_struct() {
	struct Item {
		char name[10];
		int price;
	};

	// struct Item item = {"banana", 2}
}

int main() {

	test_int();

	return 0;
}