all: Server.o
	g++ -g -o main main.cpp Server.o -lpthread

Server.o: Server.hpp Server.cpp
	g++ -g -c -o Server.o Server.cpp

clean:
	rm *.o
	rm main
