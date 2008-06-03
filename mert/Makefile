OBJS= Util.o Timer.o \
ScoreStats.o ScoreArray.o ScoreData.o \
FeatureStats.o FeatureArray.o FeatureData.o \
Data.o \
BleuScorer.o \
Point.o \
PerScorer.o \
Scorer.o \
Optimizer.o \

ifndef DEBUG
CFLAGS=-O3 -DTRACE_ENABLE -Wall
else
CFLAGS=-DTRACE_ENABLE -g -DDEBUG -Wall

endif

GCC=g++
LDFLAGS=
LDLIBS=-lm -lz 

all: \
extractor \
test_scorer \
mert \

clean: 
	rm -f *.o

%.o : %.cpp
	$(GCC) -c $(CFLAGS) $< -o $@

extractor: $(OBJS) extractor.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

mert: $(OBJS) mert.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

test_scorer: $(OBJS) test_scorer.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp
