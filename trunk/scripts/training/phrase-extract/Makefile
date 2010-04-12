
all: extract score statistics

clean: 
	rm -f *.o

.cpp.o:
	$(CXX) -O6 -g -c $<

extract: tables-core.o extract.o SyntaxTree.o XmlTree.o SentenceAlignment.o Global.o
	$(CXX) $^ -o extract

score: tables-core.o AlignmentPhrase.o score.o
	$(CXX) $^ -o score

statistics: tables-core.o AlignmentPhrase.o statistics.o
	$(CXX) $^ -o statistics
