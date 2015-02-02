#include "UnalignedWordCountFeature.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/StaticData.h"
#include "moses/Util.h"

namespace Moses
{

using namespace std;

UnalignedWordCountFeature::UnalignedWordCountFeature(const std::string &line)
  : StatelessFeatureFunction(2, line)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done." << std::endl);
}

void UnalignedWordCountFeature::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  const size_t ffScoreIndex(scoreBreakdown.GetIndexes(this).first);

  const AlignmentInfo &alignmentInfo = targetPhrase.GetAlignTerm();
  const size_t sourceLength = source.GetSize();
  const size_t targetLength = targetPhrase.GetSize();

  std::vector<bool> alignedSource(sourceLength, false);
  std::vector<bool> alignedTarget(targetLength, false);

  for (AlignmentInfo::const_iterator alignmentPoint = alignmentInfo.begin(); alignmentPoint != alignmentInfo.end(); ++alignmentPoint) 
  {
    alignedSource[ alignmentPoint->first ] = true;
    alignedTarget[ alignmentPoint->second ] = true;
  }

  size_t sourceUnalignedCount = 0;

  for (size_t j=0; j<sourceLength; ++j) {
    if (!alignedSource[j]) {
      if (!source.GetWord(j).IsNonTerminal()) {
        ++sourceUnalignedCount;
      }
    }
  }

  size_t targetUnalignedCount = 0;

  for (size_t i=0; i<targetLength; i++) {
    if (!alignedTarget[i]) {
      if (!targetPhrase.GetWord(i).IsNonTerminal()) {
        ++targetUnalignedCount;
      }
    }
  }

  scoreBreakdown.PlusEquals(ffScoreIndex, sourceUnalignedCount);
  scoreBreakdown.PlusEquals(ffScoreIndex+1, targetUnalignedCount);

  IFFEATUREVERBOSE(2) {
    FEATUREVERBOSE(2, source << std::endl);
    FEATUREVERBOSE(2, targetPhrase << std::endl);

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignTerm().begin();
         it!=targetPhrase.GetAlignTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignTerm " << it->first << " " << it->second << std::endl);
    }

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignNonTerm().begin();
         it!=targetPhrase.GetAlignNonTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignNonTerm " << it->first << " " << it->second << std::endl);
    }

    FEATUREVERBOSE(2, "sourceLength= " << sourceLength << std::endl);
    FEATUREVERBOSE(2, "targetLength= " << targetLength << std::endl);
    FEATUREVERBOSE(2, "sourceUnalignedCount= " << sourceUnalignedCount << std::endl);
    FEATUREVERBOSE(2, "targetUnalignedCount= " << targetUnalignedCount << std::endl);
  }
}

}
