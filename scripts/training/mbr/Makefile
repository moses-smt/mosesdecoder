CC=g++
OBJS=mbr.o 
CFLAGS=-O3
LDFLAGS=
LDLIBS= 

all: mbr

clean: 
	rm -f *.o

mert: $(OBJS)
	gcc $(OBJS) $(LDLIBS) -o $@

mert_p: $(OBJS)
	gcc $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
