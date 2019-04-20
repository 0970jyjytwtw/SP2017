.PHONY: all clean

# compiler name
CC=gcc

# place options like -O2, -g here
CFLAGS= -O2 -ldl -std=c99

all: inf-bonbon-server

inf-bonbon-server: MP4main.c cJSON.h cJSON.c
	$(CC) -o inf-bonbon-server $(CFLAGS) MP4main.c cJSON.h cJSON.c

clean:
	rm -f inf-bonbon-server

