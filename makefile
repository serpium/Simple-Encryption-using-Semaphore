all: encrypt

encrypt: main.o
	g++ -g -o encrypt main.o -lpthread
main.o: main.cpp
	g++ -g -c main.cpp -lpthread
clean:
	rm -f encrypt main.o
