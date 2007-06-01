
all: extract score

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o
	g++ tables-core.o extract.o -o extract

score: tables-core.o AlignmentPhrase.o score.o
	g++ tables-core.o AlignmentPhrase.o score.o -o score

