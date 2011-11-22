
all: score

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

score: score.cpp reordering_classes.o
	$(CXX) score.cpp reordering_classes.o -lz -o score

#reordering_classes.o: reordering_classes.h reordering_classes.cpp
#	$(CXX) reordering_classes.cpp

