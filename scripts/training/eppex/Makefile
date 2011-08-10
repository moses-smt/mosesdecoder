# eppex makefile

ifeq ($(DEBUG), 1)
ifeq ($(USE_UNORDERED_MAP), 1)
	CFLAGS = -g -DDEBUG -DUSE_UNORDERED_MAP
	OUT = eppex-hashing-dbg
else
	CFLAGS = -g -DDEBUG
	OUT = eppex-dbg
endif
else
ifeq ($(USE_UNORDERED_MAP), 1)
	CFLAGS = -O6 -g -DUSE_UNORDERED_MAP
	OUT = eppex-hashing
else
	CFLAGS = -O6 -g
	OUT = eppex
endif
endif

CXX = g++ $(CFLAGS) -Wall

all: eppex

clean: 
	rm -f *.o ../phrase-extract/*.o

.cpp.o:
	$(CXX) -c $*.cpp -o $*.o

counter-phrase-extract.o:
	$(CXX) -DGET_COUNTS_ONLY -c phrase-extract.cpp -o $*.o

counter.o:
	$(CXX) -DGET_COUNTS_ONLY -c $*.cpp -o $*.o

counter: ../phrase-extract/tables-core.o ../phrase-extract/SentenceAlignment.o shared.o counter-phrase-extract.o counter.o
	$(CXX) $^ -o counter

eppex: ../phrase-extract/tables-core.o ../phrase-extract/SentenceAlignment.o shared.o phrase-extract.o eppex.o
	$(CXX) $^ -o $(OUT)
