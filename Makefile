CC = gcc
STRICT = -ansi -pedantic
DIFF = cmp -bl

all: dotU

dotU: dotU.c test.c
	$(CC) $(STRICT) test.c dotU.c -o dotU

test: dotU
	./dotU test/dotu-f1 > test/dotu-f1-out
	$(DIFF) test/dotu-f1 test/dotu-f1-t0
	$(DIFF) test/dotu-f1-t1 test/dotu-f1-t0
	./dotU test/dotu-f2 > test/dotu-f2-out
	$(DIFF) test/dotu-f2 test/dotu-f2-t0
	$(DIFF) test/dotu-f2-t1 test/dotu-f2-t0
	./dotU test/dotu-f0 > test/dotu-f0-out
	$(DIFF) test/dotu-f0 test/dotu-f0-t0
	$(DIFF) test/dotu-f0-t1 test/dotu-f0-t0
	./dotU test/dotu-fbig > test/dotu-fbig-out
	$(DIFF) test/dotu-fbig test/dotu-fbig-t0
	$(DIFF) test/dotu-fbig-t1 test/dotu-fbig-t0


clean:
	rm -f *.o *.out dotU