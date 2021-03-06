# Compiler opts
CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include -I./update_ll -I./chat_ll -I./like_ll -I./room_ll -I ./client_ll -I./name_ll

# Spread opts
SP_LIBRARY_DIR=/home/cs437/exercises/ex3
LINK_SPREAD=$(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

# Our objects files. Message and linked lists
OBJECTS=message.o lts_utils.o update_ll/update_ll.o chat_ll/chat_ll.o like_ll/like_ll.o room_ll/room_ll.o client_ll/client_ll.o name_ll/name_ll.o


all: chat_server chat_client 

.o: 
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

chat_server:  $(SP_LIBRARY_DIR)/libspread-core.a $(OBJECTS) chat_server.o 
	    $(LD) -o $@ chat_server.o $(OBJECTS) $(LINK_SPREAD)

chat_client:  $(SP_LIBRARY_DIR)/libspread-core.a $(OBJECTS) chat_client.o
	$(LD) -o $@ chat_client.o $(OBJECTS) $(LINK_SPREAD)


clean:
	rm -f *.o */*.o chat_server chat_client

