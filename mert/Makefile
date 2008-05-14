OBJS= Util.o Timer.o Parameter.o \
ScoreStats.o ScoreArray.o ScoreData.o \
FeatureStats.o FeatureArray.o FeatureData.o

CFLAGS=-O3 -DTRACE_ENABLE
GCC=g++
LDFLAGS=
LDLIBS=-lm 

all: main feature_extractor

clean: 
	rm -f *.o

%.o : %.cpp %.h
	$(GCC) -c $(CFLAGS) $< -o $@

feature_extractor: $(OBJS) feature_extractor.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

main: $(OBJS) main.cpp
	$(GCC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $@ $@.cpp

