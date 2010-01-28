OBJS=mert.o data.o point.o score.o
CFLAGS=-O3
LDFLAGS=
LDLIBS=-lm 

all: mert

clean: 
	rm -f *.o

mert: $(OBJS)
	$(CXX) $(OBJS) $(LDLIBS) -o $@

mert_p: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
