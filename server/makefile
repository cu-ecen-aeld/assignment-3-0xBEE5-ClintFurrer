CROSS_COMPILE ?= 
CC=$(CROSS_COMPILE)gcc
CFLAGS=-g -Wall -Werror

# The 'all' target makes 'aesdsocket' the default build goal
all: aesdsocketmake

aesdsocketmake: aesdsocket.o
	$(CC) -o aesdsocket aesdsocket.o

# Rule to compile the .c file into a .o file
writer.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o

.PHONY: clean
clean:
	rm -f *.o aesdsocket *~ core 
