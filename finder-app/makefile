CROSS_COMPILE ?= 
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror

TARGET ?= writer

SRCS = writer.c
OBJS ?= $(SRCS:.c=.o)
LDFLAGS ?= -lpthread -lrt

# The 'all' target makes 'writer' the default build goal
all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile the .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile the .c file into a .o file
#writer.o: writer.c
#	$(CC) $(CFLAGS) -c writer.c -o writer.o

.PHONY: clean
clean:
	rm -f *.o writer *~ core 
