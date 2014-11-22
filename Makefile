CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: spread_tester 

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

spread_tester:  $(SP_LIBRARY_DIR)/libspread-core.a spread_tester.o
	$(LD) -o $@ spread_tester.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean:
	rm -f *.o spread_tester

