all: extract score consolidate relax-parse consolidate-direct delete-unary swap-alignment

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o ExtractedRule.o Hole.o HoleCollection.o SentenceAlignment.o Global.o InputFileStream.o
	g++ tables-core.o extract.o SyntaxTree.o XmlTree.o ExtractedRule.o Hole.o HoleCollection.o SentenceAlignment.o  Global.o InputFileStream.o -lz -o extract

score: tables-core.o AlignmentPhrase.o score.o InputFileStream.o
	g++ tables-core.o AlignmentPhrase.o score.o InputFileStream.o -lz -o score

consolidate: consolidate.o InputFileStream.o
	g++ consolidate.o InputFileStream.o -lz -o consolidate

relax-grammar: tables-core.o SyntaxTree.o XmlTree.o relax-grammar.o relax-grammar.h
	g++ tables-core.o SyntaxTree.o XmlTree.o relax-grammar.o -o relax-grammar

consolidate-direct: consolidate-direct.o InputFileStream.o
	g++ consolidate-direct.o InputFileStream.o -lz -o consolidate-direct

relax-parse: tables-core.o SyntaxTree.o XmlTree.o relax-parse.o relax-parse.h
	g++ tables-core.o SyntaxTree.o XmlTree.o relax-parse.o -o relax-parse

delete-unary: delete-unary.o
	g++ delete-unary.o -o delete-unary

swap-alignment: swap-alignment.o
	g++ swap-alignment.o -o swap-alignment