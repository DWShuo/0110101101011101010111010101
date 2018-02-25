#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "heapq.h"

int cmp_int(void *_p1, void *_p2) {
	int32_t *p1 = (int32_t*)_p1;
	int32_t *p2 = (int32_t*)_p2;
	if (*p1 < *p2)
		return -1;
	if (*p1 == *p2)
		return 0;
	else
		return 1;
}

void test_int() {
	heapq_t h;
	int32_t popped;
	int32_t tmp;
	heap_init(&h, cmp_int);

	int item = 4;
	heap_push(&h, &item);
	popped = *(int32_t*)heap_pop(&h);
	printf("%d\n", popped);
	assert(popped == 4);

	int items[10] = {1, 2, 4, 7, 8, 3, 9, 0, 6, 5};
	for (int i = 0; i < 10; ++i) {
		heap_push(&h, items+i);
	}

	popped = 0;
	for (int i = 0; i < 10; ++i) {
		tmp = popped;
		popped = *(int32_t*)heap_pop(&h);
		printf("popped: %d   tmp: %d\n", popped, tmp);
		assert(tmp <= popped);
	}

	int items2[10] = {4, 4, 4, 6, 7, -1, 2, 2, -100, 7};
	for (int i = 0; i < 10; ++i) {
		heap_push(&h, items2+i);
	}

	popped = -200;
	for (int i = 0; i < 10; ++i) {
		tmp = popped;
		popped = *(int32_t*)heap_pop(&h);
		printf("popped: %d   tmp: %d\n", popped, tmp);
		assert(tmp <= popped);
	}
}

struct Item {
	char name[20];
	int32_t price;
};

int cmp_struct_item(void *_p1, void *_p2) {
	struct Item p1 = *(struct Item*)_p1;
	struct Item p2 = *(struct Item*)_p2;
	if (p1.price < p2.price)
		return -1;
	else if (p1.price == p2.price)
		return 0;
	else
		return 1;
}

void test_struct() {
	heapq_t h;
	struct Item popped;
	struct Item tmp;
	heap_init(&h, cmp_struct_item);

	struct Item item = {"banana", 2};
	heap_push(&h, &item);
	popped = *(struct Item*)heap_pop(&h);
	printf("name: %s   price: %d\n", popped.name, popped.price);
	assert(strlen(popped.name) == 6);
	assert(popped.price == 2);

	struct Item i2 = {"apple", 2};
	heap_push(&h, &i2);
	struct Item i3 = {"pear", 3};
	heap_push(&h, &i3);
	struct Item i4 = {"raspberries", 5};
	heap_push(&h, &i4);
	struct Item i5 = {"blueberries", 4};
	heap_push(&h, &i5);
	struct Item i6 = {"lemon", 2};
	heap_push(&h, &i6);

	popped.name[0] = '\0';
	popped.price = 0;
	for (int i = 0; i < 5; ++i) {
		tmp = popped;
		popped = *(struct Item*)heap_pop(&h);
		printf("name: %s   price: %d\n", popped.name, popped.price);
		assert(tmp.price <= popped.price);
	}

}

int main() {

	test_int();

	test_struct();

	return 0;
}