CC=gcc
LD=gcc
CFLAGS=-g -Wall -fdiagnostics-color=always -O3
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

all: spread_tester chat_server chat_client ll_test

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

chat_server:  $(SP_LIBRARY_DIR)/libspread-core.a chat_server.o
	$(LD) -o $@ chat_server.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


chat_client:  $(SP_LIBRARY_DIR)/libspread-core.a chat_client.o
	$(LD) -o $@ chat_client.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


spread_tester:  $(SP_LIBRARY_DIR)/libspread-core.a spread_tester.o
	$(LD) -o $@ spread_tester.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a


ll_test: update_ll/update_ll.o update_ll/test_update_ll.o
	$(LD) -o $@ update_ll/test_update_ll.o update_ll/update_ll.o

clean:
	rm -f *.o 	update_ll/*.o ll_test spread_tester chat_server chat_client
>>>>>>> 7175d6bbfb3d99a4394267e14385eeacba2c29ee

