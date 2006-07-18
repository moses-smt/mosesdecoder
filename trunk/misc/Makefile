
CXX=g++
CXXFLAGS=-W -Wall -O0 -g -ggdb -ffor-scope
LDFLAGS=-static
INCLUDES=-I../moses/src

default: processPhraseTable

processPhraseTable.o: processPhraseTable.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@

MOSESLIB =$(HOME)/workspace/moses/src/libmoses.a

processPhraseTable: processPhraseTable.o $(MOSESLIB)
	$(CXX) $(LDFLAGS) $^ -o $@




