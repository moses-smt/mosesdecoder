all: consolidate consolidate-direct consolidate-reverse extract extract-rules relax-parse \
     score statistics extract-lex

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

extract: tables-core.o SentenceAlignment.o extract.o
	$(CXX) $^ -o extract

extract-rules: tables-core.o SentenceAlignment.o SentenceAlignmentWithSyntax.o SyntaxTree.o XmlTree.o HoleCollection.o extract-rules.o ExtractedRule.o
	$(CXX) $^ -o extract-rules

extract-lex: extract-lex.o InputFileStream.o
	$(CXX) $^ -lz -o extract-lex

score: tables-core.o AlignmentPhrase.o score.o PhraseAlignment.o InputFileStream.o
	$(CXX) $^ -lz -o score

consolidate: consolidate.o tables-core.o InputFileStream.o
	$(CXX) $^ -lz -o consolidate

consolidate-direct: consolidate-direct.o InputFileStream.o
	$(CXX) $^ -lz -o consolidate-direct

consolidate-reverse: consolidate-reverse.o tables-core.o InputFileStream.o
	$(CXX) $^ -lz -o consolidate-reverse

relax-parse: tables-core.o SyntaxTree.o XmlTree.o relax-parse.o
	$(CXX) $^ -o relax-parse

statistics: tables-core.o AlignmentPhrase.o statistics.o
	$(CXX) $^ -o statistics


