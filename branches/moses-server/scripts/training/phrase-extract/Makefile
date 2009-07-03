
all: extract score

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

extract: tables-core.o extract.o
	$(CXX) tables-core.o extract.o -o extract

score: tables-core.o AlignmentPhrase.o score.o
	$(CXX) tables-core.o AlignmentPhrase.o score.o -o score

