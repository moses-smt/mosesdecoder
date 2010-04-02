CXXFLAGS=-O3
LDFLAGS=
LDLIBS= 

all: mbr

clean: 
	rm -f *.o

mert: $(OBJS)
	$(G++) $(OBJS) $(LDLIBS) -o $@

mert_p: $(OBJS)
	$(G++) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
