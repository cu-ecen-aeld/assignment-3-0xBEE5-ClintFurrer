CROSS_COMPILE ?= 
CC=$(CROSS_COMPILE)gcc
CFLAGS= -g -Wall -Werror

TARGET = aesdsocket

SRCS = aesdsocket.c
OBJS = $(SRCS:.c=.o)
DEPS = aesdsocket.h


# The 'all' target makes 'aesdsocket' the default build goal
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

# Rule to compile the .c file into a .o file
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
