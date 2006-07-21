

BOOSTDIR=/home/ws06/cdyer/boost-stage
CXX=g++
CXXFLAGS=-W -Wall -O0 -g -ggdb -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LDFLAGS=-static
INCLUDES=-I../moses/src -I$(BOOSTDIR) -I$(BOOSTDIR)/include
LIBS=-L$(BOOSTDIR)/lib -L$(BOOSTDIR)/stage/lib -lboost_iostreams-gcc-mt -lboost_filesystem-gcc-mt -lboost_thread-gcc-mt -lz


default: processPhraseTable

processPhraseTable.o: processPhraseTable.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@

MOSESLIB =$(HOME)/workspace/moses/src/libmoses.a

processPhraseTable: processPhraseTable.o $(MOSESLIB)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)




