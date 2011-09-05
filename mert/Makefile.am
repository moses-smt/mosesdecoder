bin_PROGRAMS = mert extractor evaluator
if WITH_TER
  TERSOURCES = \
    TERsrc/alignmentStruct.cpp \
    TERsrc/hashMap.cpp \
    TERsrc/hashMapStringInfos.cpp \
    TERsrc/segmentStructure.cpp \
    TERsrc/stringHasher.cpp \
    TERsrc/terAlignment.cpp \
    TERsrc/terShift.cpp \
    TERsrc/tinyxml.cpp \
    TERsrc/tinyxmlparser.cpp \
    TERsrc/documentStructure.cpp \
    TERsrc/hashMapInfos.cpp \
    TERsrc/infosHasher.cpp \
    TERsrc/stringInfosHasher.cpp \
    TERsrc/tercalc.cpp \
    TERsrc/tinystr.cpp \
    TERsrc/tinyxmlerror.cpp \
    TERsrc/tools.cpp \
    TerScorer.cpp 
endif
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
mert.cpp \
CderScorer.cpp $(TERSOURCES)

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
extractor.cpp \
CderScorer.cpp $(TERSOURCES)

evaluator_SOURCES = Util.cpp \
evaluator.cpp \
Timer.cpp \
ScoreStats.cpp ScoreArray.cpp ScoreData.cpp \
BleuScorer.cpp \
PerScorer.cpp \
Scorer.cpp \
CderScorer.cpp $(TERSOURCES)

mert_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE
extractor_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE
evaluator_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE

extractor_LDADD = -lm -lz
mert_LDADD = -lm -lz
evaluator_LDADD = -lm -lz
