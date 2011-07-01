all: consolidate consolidate-direct extract extract-rules relax-parse \
     score statistics extract-lex

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

extract: tables-core.o SentenceAlignment.o extract.o
	$(CXX) $^ -o extract

extract-rules: tables-core.o SentenceAlignment.o SentenceAlignmentWithSyntax.o SyntaxTree.o XmlTree.o HoleCollection.o extract-rules.o 
	$(CXX) $^ -o extract-rules

extract-lex: extract-lex.o
	$(CXX) $^ -o extract-lex

score: tables-core.o AlignmentPhrase.o score.o PhraseAlignment.o InputFileStream.o
	$(CXX) $^ -lz -o score

consolidate: consolidate.o tables-core.o
	$(CXX) $^ -o consolidate

consolidate-direct: consolidate-direct.o InputFileStream.o
	$(CXX) $^ -lz -o consolidate-direct

relax-parse: tables-core.o SyntaxTree.o XmlTree.o relax-parse.o
	$(CXX) $^ -o relax-parse

statistics: tables-core.o AlignmentPhrase.o statistics.o
	$(CXX) $^ -o statistics


