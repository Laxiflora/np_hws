all: socks_server.cpp console.cpp
	g++ socks_server.cpp -O0 -o socks_server -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system
	g++ console.cpp -o hw4.cgi -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system