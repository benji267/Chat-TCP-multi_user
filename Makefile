CC=gcc
CFLAGS= -o
LDFLAGS=-g -lpthread -lrt

all: sender server

sender: sender.c
	$(CC) $(CFLAGS) $(LDFLAGS) sender.c -o sender -DPEERTOPEER

server: server.c
	$(CC) $(CFLAGS) $(LDFLAGS) server.c -o server -pthread -DPEERTOPEER

clean:
	rm -f sender server