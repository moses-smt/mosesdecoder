all: biconcor

clean: 
	rm -f *.o

.cpp.o:
	g++ -O6 -g -c $<

biconcor: Vocabulary.o SuffixArray.o TargetCorpus.o Alignment.o PhrasePair.o PhrasePairCollection.o biconcor.o
	g++ Vocabulary.o SuffixArray.o TargetCorpus.o Alignment.o PhrasePair.o PhrasePairCollection.o biconcor.o -o biconcor
