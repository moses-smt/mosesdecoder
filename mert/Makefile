OBJS= Util.o Timer.o Parameter.o \
ScoreStats.o ScoreArray.o ScoreData.o \
FeatureStats.o FeatureArray.o FeatureData.o \
Data.o \
BleuScorer.o \
Point.o \
Optimizer.o

ifndef DEBUG
CFLAGS=-O3 -DTRACE_ENABLE
else
CFLAGS=-DTRACE_ENABLE -g 
endif

GCC=g++
LDFLAGS=
LDLIBS=-lm 

all: \
mert \
feature_extractor \
extractor \
test_scorer

clean: 
	rm -f *.o

%.o : %.cpp %.h
	$(GCC) -c $(CFLAGS) $< -o $@

feature_extractor: $(OBJS) feature_extractor.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

extractor: $(OBJS) extractor.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

main: $(OBJS) mert.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

test_scorer: $(OBJS) test_scorer.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

