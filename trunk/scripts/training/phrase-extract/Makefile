
all: extract extract-rules score consolidate consolidate-direct statistics

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

extract: tables-core.o SentenceAlignment.o extract.o
	$(CXX) $^ -o extract

extract-rules: tables-core.o SentenceAlignment.o SentenceAlignmentWithSyntax.o SyntaxTree.o XmlTree.o HoleCollection.o extract-rules.o 
	$(CXX) $^ -o extract-rules

score: tables-core.o AlignmentPhrase.o score.o
	$(CXX) $^ -o score

consolidate: consolidate.o
	$(CXX) $^ -o consolidate

consolidate-direct: consolidate-direct.o
	$(CXX) $^ -o consolidate-direct

statistics: tables-core.o AlignmentPhrase.o statistics.o
	$(CXX) $^ -o statistics
