all: manual-label

clean: 
	rm -f *.o manual-label

.cpp.o:
	g++ -I../../../ -O6 -g -c $<

manual-label: DeEn.o manual-label.o

	g++ DeEn.o manual-label.o -lz -lboost_program_options-mt -o manual-label


