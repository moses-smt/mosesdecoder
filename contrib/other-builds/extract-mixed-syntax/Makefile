all: extract 

clean: 
	rm -f *.o extract-mixed-syntax

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o Tunnel.o Lattice.o LatticeNode.o SentenceAlignment.o Global.o InputFileStream.o TunnelCollection.o RuleCollection.o Rule.o Symbol.o SymbolSequence.o Range.o

	g++ tables-core.o extract.o SyntaxTree.o XmlTree.o Tunnel.o Lattice.o LatticeNode.o SentenceAlignment.o Global.o InputFileStream.o TunnelCollection.o RuleCollection.o Rule.o Symbol.o SymbolSequence.o Range.o -lz -o extract-mixed-syntax

