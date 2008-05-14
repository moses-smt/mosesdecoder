OBJS= Util.o Timer.o Parameter.o \
ScoreStats.o ScoreArray.o ScoreData.o \
FeatureStats.o FeatureArray.o FeatureData.o \
BleuScorer.o \
Point.o Optimizer.o

CFLAGS=-O3 -DTRACE_ENABLE
GCC=g++
LDFLAGS=
LDLIBS=-lm 

all: main \
feature_extractor \
test_scorer

clean: 
	rm -f *.o

%.o : %.cpp %.h
	$(GCC) -c $(CFLAGS) $< -o $@

feature_extractor: $(OBJS) feature_extractor.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

main: $(OBJS) main.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

test_scorer: $(OBJS) test_scorer.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

