CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include -I./update_ll
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: chat_server chat_client ll_test

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

chat_server:  $(SP_LIBRARY_DIR)/libspread-core.a chat_server.o utils.h update_ll/update_ll.o
	$(LD) -o $@ chat_server.o update_ll/update_ll.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


chat_client:  $(SP_LIBRARY_DIR)/libspread-core.a chat_client.o utils.h
	$(LD) -o $@ chat_client.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


ll_test: update_ll/update_ll.o update_ll/test_update_ll.o
	$(LD) -o $@ update_ll/test_update_ll.o update_ll/update_ll.o

clean:
	rm -f *.o 	update_ll/*.o ll_test chat_server chat_client

