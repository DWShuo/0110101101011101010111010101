#ifndef HEAPQ_H
#define HEAPQ_H

#define MAXNAME 8
#define MAXQUEUE 100

typedef struct {
	void *elements[MAXQUEUE];
	int length;
	int capacity;
	int (*cmp)(void *, void *);  // comparison function for ordering
} heapq_t;

// cmp is the comparison function for ordering
void heap_init(heapq_t *heap, int (*cmp)(void *, void *));
void heap_push(heapq_t *heap, void *item);
void *heap_pop(heapq_t *heap);

#endif
