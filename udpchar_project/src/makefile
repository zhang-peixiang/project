all:ChatServer ChatClient

ChatServer:ChatServer.cpp
	g++ $^ -o $@ -g -lpthread -std=c++11 -ljsoncpp

ChatClient:ChatClient.cpp
	g++ $^ -o $@ -g -lpthread -std=c++11 -ljsoncpp
