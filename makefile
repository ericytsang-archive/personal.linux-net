
CC = g++ -Wall -W -Wextra -std=c++11
LIBS = -pthread



# clean
clean:
	rm -R *.out *.o



# client test modules
ClientTest: ./ClientTest.o ./Host.o ./select_helper.o ./net_helper.o
	$(CC) $(LIBS) -o ./ClientTest.out ./ClientTest.o ./Host.o ./select_helper.o ./net_helper.o

ClientTest.o: ./ClientTest.cpp
	$(CC) -c ./ClientTest.cpp




# server test modules
ServerTest: ./ServerTest.o ./Host.o ./select_helper.o ./net_helper.o
	$(CC) $(LIBS) -o ./ServerTest.out ./ServerTest.o ./Host.o ./select_helper.o ./net_helper.o

ServerTest.o: ./ServerTest.cpp
	$(CC) -c ./ServerTest.cpp



# client test modules
Client: ./Client.o ./Host.o ./select_helper.o ./net_helper.o
	$(CC) $(LIBS) -o ./Client.out ./Client.o ./Host.o ./select_helper.o ./net_helper.o

Client.o: ./Client.cpp
	$(CC) -c ./Client.cpp




# server test modules
Server: ./Server.o ./Host.o ./select_helper.o ./net_helper.o
	$(CC) $(LIBS) -o ./Server.out ./Server.o ./Host.o ./select_helper.o ./net_helper.o

Server.o: ./Server.cpp
	$(CC) -c ./Server.cpp




# shared helper modules
select_helper.o: ./select_helper.cpp ./select_helper.h
	$(CC) -c ./select_helper.cpp

net_helper.o: ./net_helper.cpp ./net_helper.h
	$(CC) -c ./net_helper.cpp

Host.o: ./Host.cpp ./Host.h
	$(CC) -c ./Host.cpp
