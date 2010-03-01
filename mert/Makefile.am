bin_PROGRAMS = mert extractor
mert_SOURCES = Util.cpp \
     Timer.cpp \
ScoreStats.cpp ScoreArray.cpp ScoreData.cpp \
FeatureStats.cpp FeatureArray.cpp FeatureData.cpp \
Data.cpp \
BleuScorer.cpp \
Point.cpp \
PerScorer.cpp \
Scorer.cpp \
Optimizer.cpp \
mert.cpp

extractor_SOURCES = Util.cpp \
     Timer.cpp \
ScoreStats.cpp ScoreArray.cpp ScoreData.cpp \
FeatureStats.cpp FeatureArray.cpp FeatureData.cpp \
Data.cpp \
BleuScorer.cpp \
Point.cpp \
PerScorer.cpp \
Scorer.cpp \
Optimizer.cpp \
extractor.cpp


mert_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE
extractor_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE

extractor_LDADD = -lm -lz
mert_LDADD = -lm -lz
