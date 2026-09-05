CROSS_COMPILE ?= 
CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -Wall

# The 'all' target makes 'writer' the default build goal
all: writermake

writermake: writer.o
	$(CC) -o writer writer.o

# Rule to compile the .c file into a .o file
writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c -o writer.o

.PHONY: clean
clean:
	rm -f *.o writer *~ core 
