all: extract 

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o Tunnel.o Lattice.o LatticeNode.o SentenceAlignment.o Global.o InputFileStream.o TunnelCollection.o RuleCollection.o Rule.o Symbol.o SymbolSequence.o

	g++ tables-core.o extract.o SyntaxTree.o XmlTree.o Tunnel.o Lattice.o LatticeNode.o SentenceAlignment.o Global.o InputFileStream.o TunnelCollection.o RuleCollection.o Rule.o Symbol.o SymbolSequence.o -lz -o extract

