all: suffix-test fuzzy-match

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

suffix-test: Vocabulary.o SuffixArray.o suffix-test.o
	g++ Vocabulary.o SuffixArray.o suffix-test.o -o suffix-test

fuzzy-match: Vocabulary.o SuffixArray.o fuzzy-match.o
	g++ Vocabulary.o SuffixArray.o fuzzy-match.o -o fuzzy-match

fuzzy-match2: Vocabulary.o SuffixArray.o fuzzy-match2.o
	g++ Vocabulary.o SuffixArray.o fuzzy-match2.o -o fuzzy-match2
