

BOOSTDIR=/home/ws06/cdyer/boost-stage
SRIDIR=/home/ws06/cdyer/srilm/lib/i686
CXX=g++
CXXFLAGS=-W -Wall -O0 -g -ggdb -ffor-scope -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES
LDFLAGS=-static
INCLUDES=-I../moses/src -I$(BOOSTDIR) -I$(BOOSTDIR)/include
BOOSTLIBS=-L$(BOOSTDIR)/lib -L$(BOOSTDIR)/stage/lib -lboost_iostreams-gcc-mt -lboost_filesystem-gcc-mt -lboost_thread-gcc-mt -lz
SRILIBS=-L$(SRIDIR) -loolm -ldstruct -lmisc

default: processPhraseTable

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@



MOSESLIB =../moses/src/libmoses.a

processPhraseTable: processPhraseTable.o GenerateTuples.o $(MOSESLIB)
	$(CXX) $(LDFLAGS) $^ -o $@ $(SRILIBS) $(BOOSTLIBS)




