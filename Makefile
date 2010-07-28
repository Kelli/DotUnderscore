CC = gcc
STRICT = -ansi -pedantic

all: dotU

dotU: dotU.c
	$(CC) $(STRICT) dotU.c -o dotU

clean:
	rm -f *.o dotU