#include <assert.h>
#include "heapq.h"

static void siftdown(heapq_t *heap, int startpos, int pos) {
	void *newitem = heap->elements[pos];
	while (pos > startpos) {
		int parentpos = (pos - 1) >> 1;
		void *parent = heap->elements[parentpos];
		if (heap->cmp(newitem, parent) < 0) {
			heap->elements[pos] = parent;
			pos = parentpos;
			continue;
		}
		break;
	}
	heap->elements[pos] = newitem;
}

static void siftup(heapq_t *heap, int pos) {
	int endpos = heap->length;
	int startpos = pos;
	void *newitem = heap->elements[pos];
	int childpos = 2*pos + 1;
	while (childpos < endpos) {
		int rightpos = childpos + 1;
		void *child = heap->elements[childpos];
		void *right = heap->elements[rightpos];
		if (rightpos < endpos && heap->cmp(child, right) >= 0) {
			childpos = rightpos;
		}
		heap->elements[pos] = heap->elements[childpos];
		pos = childpos;
		childpos = 2*pos + 1;
	}
	heap->elements[pos] = newitem;
	siftdown(heap, startpos, pos);
}

void heap_init(heapq_t *heap, int (*cmp)(void *, void *)) {
	heap->length = 0;
	heap->capacity = MAXQUEUE;
	heap->cmp = cmp;
}

void heap_push(heapq_t *heap, void *item) {
	assert(heap->length < MAXQUEUE);
	heap->elements[heap->length++] = item;
	siftdown(heap, 0, heap->length-1);
}

void *heap_pop(heapq_t *heap) {
	assert(heap->length > 0);
	void *last = heap->elements[--(heap->length)];
	if (heap->length > 0) {
		void *ret = heap->elements[0];
		heap->elements[0] = last;
		siftup(heap, 0);
		return ret;
	}
	return last;
}
