
all: extract score consolidate

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
