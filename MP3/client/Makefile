.PHONY: all clean

# compiler name
CC=gcc

# place options like -O2, -g here
CFLAGS=-O2 -I../server

all: miner

miner: miner.c
	$(CC) -o miner $(CFLAGS) miner.c

clean:
	rm -f miner
