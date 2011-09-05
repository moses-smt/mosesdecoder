bin_PROGRAMS = mert extractor evaluator
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
CderScorer.cpp

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
CderScorer.cpp

evaluator_SOURCES = Util.cpp \
evaluator.cpp \
Timer.cpp \
ScoreStats.cpp ScoreArray.cpp ScoreData.cpp \
BleuScorer.cpp \
PerScorer.cpp \
Scorer.cpp \
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
CderScorer.cpp

mert_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE
extractor_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE
evaluator_CPPFLAGS = -W -Wall -Wno-unused -ffor-scope -DTRACE_ENABLE

extractor_LDADD = -lm -lz
mert_LDADD = -lm -lz
evaluator_LDADD = -lm -lz
