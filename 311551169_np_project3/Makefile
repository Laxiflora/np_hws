all: http_server.cpp console.cpp cgi_server.cpp
	g++ http_server.cpp -o http_server -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system
	g++ console.cpp -o console.cgi -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system
	g++ cgi_server.cpp -o cgi_server.exe -lstdc++fs -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system -std=c++17

part1: http_server.cpp console.cpp
	g++ http_server.cpp -o http_server -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system
	g++ console.cpp -o console.cgi -I /usr/local/include -L /usr/local/lib -std=c++14 -Wall -pedantic -pthread -lboost_system

part2: cgi_server.cpp
	g++ cgi_server.cpp -o cgi_server.exe -lws2_32 -lwsock32 -std=c++17