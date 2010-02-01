all: extract score consolidate relax-parse consolidate-direct

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o ExtractedRule.o Hole.o HoleCollection.o SentenceAlignment.o Global.o
	g++ tables-core.o extract.o SyntaxTree.o XmlTree.o ExtractedRule.o Hole.o HoleCollection.o SentenceAlignment.o  Global.o -o extract

score: tables-core.o AlignmentPhrase.o score.o 
	g++ tables-core.o AlignmentPhrase.o score.o -o score

consolidate: consolidate.o
	g++ consolidate.o -o consolidate

relax-grammar: tables-core.o SyntaxTree.o XmlTree.o relax-grammar.o relax-grammar.h
	g++ tables-core.o SyntaxTree.o XmlTree.o relax-grammar.o -o relax-grammar

consolidate-direct: consolidate-direct.o
	g++ consolidate-direct.o -o consolidate-direct

relax-parse: tables-core.o SyntaxTree.o XmlTree.o relax-parse.o relax-parse.h
	g++ tables-core.o SyntaxTree.o XmlTree.o relax-parse.o -o relax-parse
