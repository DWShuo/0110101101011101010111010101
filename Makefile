CC=gcc
CFLAGS = -Werror -Wall

all : main

clean :
	rm main heapq.o

heapq.o : heapq.c heapq.h

main : main.c heapq.o
