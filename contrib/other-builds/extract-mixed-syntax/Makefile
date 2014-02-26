all: extract-mixed-syntax 

clean: 
	rm -f *.o extract-mixed-syntax

.cpp.o:
	g++ -O4 -g -c -I../../../boost/include -I../../../ $<

OBJECTS = AlignedSentence.o ConsistentPhrase.o ConsistentPhrases.o InputFileStream.o \
	Main.o OutputFileStream.o Parameter.o Phrase.o Rule.o Rules.o RuleSymbol.o \
	SyntaxTree.o Word.o NonTerm.o RulePhrase.o AlignedSentenceSyntax.o pugixml.o

extract-mixed-syntax: $(OBJECTS) 

	g++ $(OBJECTS)   -L../../../boost/lib64 -lz -lboost_iostreams-mt -lboost_program_options-mt -o extract-mixed-syntax


