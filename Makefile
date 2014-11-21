CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: ll_test

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

ll_test: update_ll/update_ll.o update_ll/test_update_ll.o
	$(LD) -o $@ update_ll/test_update_ll.o update_ll/update_ll.o

clean:
	rm -f *.o update_ll/*.o ll_test

