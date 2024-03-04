CC=gcc
CFLAGS=-Wall

all: server_proactor client proactor

client: client.c
	$(CC) $(CFLAGS) -o client client.c
server_proactor: server_proactor.c
	$(CC) $(CFLAGS) -o server_proactor server_proactor.c
proactor: proactor.c proactor.h
	$(CC) $(CFLAGS) -c proactor.c

clean:
	rm -f client server_proactor proactor.o

