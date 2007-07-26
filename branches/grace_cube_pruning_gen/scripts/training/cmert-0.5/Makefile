CC=gcc
OBJS=mert.o data.o point.o score.o
CFLAGS=-O3
LDFLAGS=
LDLIBS=-lm 

all: mert

clean: 
	rm -f *.o

mert: $(OBJS)
	gcc $(OBJS) $(LDLIBS) -o $@

mert_p: $(OBJS)
	gcc $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
