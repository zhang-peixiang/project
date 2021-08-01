SVR_BIN=../bin/ChatServer
CLI_BIN=../bin/ChatClient

all:$(SVR_BIN) $(CLI_BIN)
$(SVR_BIN):ChatServer.cpp
	g++ $^ -o $@ -g -lpthread -std=c++11 -ljsoncpp
	
$(CLI_BIN):ChatClient.cpp
	g++ $^ -o $@ -g -lpthread -std=c++11 -ljsoncpp -lncurses
