all: filter-pt

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

filter-pt: filter-pt.o tables-core.o Lattice.o LatticeNode.o SyntaxTree.o XmlTree.o InputFileStream.o Vocab.o
	g++ filter-pt.o tables-core.o Lattice.o LatticeNode.o SyntaxTree.o XmlTree.o InputFileStream.o Vocab.o -o filter-pt -lz
	
	