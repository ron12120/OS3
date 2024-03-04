CC=gcc
CFLAGS=-Wall

# Object file to be created
OBJ=proactor.o

# Default target
all: $(OBJ)

# Compile proactor.c into proactor.o
$(OBJ): proactor.c proactor.h
	$(CC) $(CFLAGS) -c proactor.c

clean:
	rm -f $(OBJ)

.PHONY: all clean

