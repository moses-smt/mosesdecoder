
all: extract score consolidate consolidate-direct

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o
	g++ tables-core.o extract.o SyntaxTree.o XmlTree.o -o extract

score: tables-core.o AlignmentPhrase.o score.o
	g++ tables-core.o AlignmentPhrase.o score.o -o score

consolidate: consolidate.o
	g++ consolidate.o -o consolidate

consolidate-direct: consolidate-direct.o
	g++ consolidate-direct.o -o consolidate-direct
