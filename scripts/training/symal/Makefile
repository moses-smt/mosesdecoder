CPP=g++ -g 
CC=gcc -g 

all: symal

clean: 
	rm -f *.o

cmd.o: cmd.c cmd.h
	$(CC) -c -o cmd.o cmd.c

symal: symal.cpp cmd.o
	$(CPP) -o $@ $(@).cpp cmd.o
