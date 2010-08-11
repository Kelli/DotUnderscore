CC = gcc
STRICT = -ansi -pedantic

all: dotU

dotU: dotU.c test.c
	$(CC) $(STRICT) test.c dotU.c -o dotU

test: dotU
	./dotU test/dotu-f1    > test/dotu-f1-out
	./dotU test/dotu-f2    > test/dotu-f2-out
	./dotU test/dotu-f0    > test/dotu-f0-out
	./dotU test/dotu-fbig  > test/dotu-fbig-out


clean:
	rm -f *.o *.out dotU