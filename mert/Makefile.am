lib_LTLIBRARIES = libmert.la
bin_PROGRAMS = mert extractor evaluator
AM_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE $(BOOST_CPPFLAGS)

libmert_la_SOURCES =  \
Util.cpp \
FileStream.cpp \
Timer.cpp \
ScoreStats.cpp ScoreArray.cpp ScoreData.cpp \
FeatureStats.cpp FeatureArray.cpp FeatureData.cpp \
Data.cpp \
BleuScorer.cpp \
Point.cpp \
PerScorer.cpp \
Scorer.cpp \
Optimizer.cpp \
TERsrc/alignmentStruct.cpp \
TERsrc/hashMap.cpp \
TERsrc/hashMapStringInfos.cpp \
TERsrc/stringHasher.cpp \
TERsrc/terAlignment.cpp \
TERsrc/terShift.cpp \
TERsrc/hashMapInfos.cpp \
TERsrc/infosHasher.cpp \
TERsrc/stringInfosHasher.cpp \
TERsrc/tercalc.cpp \
TERsrc/tools.cpp \
TerScorer.cpp \
CderScorer.cpp \
MergeScorer.cpp

mert_SOURCES = mert.cpp $(top_builddir)/moses/src/ThreadPool.cpp
extractor_SOURCES  = extractor.cpp
evaluator_SOURCES = evaluator.cpp

extractor_LDADD = libmert.la -lm -lz
mert_LDADD = libmert.la -lm -lz $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS)
evaluator_LDADD = libmert.la -lm -lz
