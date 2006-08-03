CC=gcc
OBJS=mert.o data.o point.o score.o
CFLAGS=-O3 -pg
LDFLAGS=-pg
LDLIBS=-lm 

mert: $(OBJS)
	gcc $(OBJS) $(LDLIBS) -o $@

mert_p: $(OBJS)
	gcc $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
