CROSS_COMPILE ?= 
CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -Wall


writermake: writer.o
	$(CC) -o writer writer.o

.PHONY: clean
clean:
	rm -f *.o writer *~ core 