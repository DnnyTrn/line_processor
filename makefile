CC=gcc
CFLAGS= -g -pthread

all: line_processor

line_processor: line_processor.c
	$(CC) $(CFLAGS) -o line_processor line_processor.c

clean:
	rm -f line_processor *.o


