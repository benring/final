CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include -I./update_ll -I./chat_ll -I./like_ll -I./room_ll -I ./client_ll
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: chat_server chat_client 

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

chat_server:  $(SP_LIBRARY_DIR)/libspread-core.a chat_server.o lts_utils.o utils.h update_ll/update_ll.o chat_ll/chat_ll.o like_ll/like_ll.o room_ll/room_ll.o client_ll/client_ll.o
	    $(LD) -o $@ chat_server.o lts_utils.o client_ll/client_ll.o update_ll/update_ll.o chat_ll/chat_ll.o like_ll/like_ll.o room_ll/room_ll.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


chat_client:  $(SP_LIBRARY_DIR)/libspread-core.a chat_client.o lts_utils.o utils.h update_ll/update_ll.o chat_ll/chat_ll.o like_ll/like_ll.o
	$(LD) -o $@ chat_client.o lts_utils.o update_ll/update_ll.o chat_ll/chat_ll.o like_ll/like_ll.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


clean:
	rm -f *.o 	update_ll/*.o update_ll_test chat_ll_test like_ll_test chat_server chat_client

