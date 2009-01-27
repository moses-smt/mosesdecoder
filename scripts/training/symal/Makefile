CPP=g++ -g 
CC=gcc -g 

all: symal

clean: 
	rm -f *.o

cmd.o: cmd.c cmd.h
	$(CC) -O3 -c -o cmd.o cmd.c

symal: symal.cpp cmd.o
	$(CPP) -O3 -o $@ $(@).cpp cmd.o
