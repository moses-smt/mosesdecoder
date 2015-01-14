#include <map>
#include <vector>
#include <assert.h>
#include "SourceGHKMTreeInputMatchFeature.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/Factor.h"
#include "moses/FactorCollection.h"
#include "moses/InputPath.h"
#include "moses/TreeInput.h"


using namespace std;

namespace Moses
{

SourceGHKMTreeInputMatchFeature::SourceGHKMTreeInputMatchFeature(const std::string &line)
  : StatelessFeatureFunction(2, line)
{
  std::cerr << GetScoreProducerDescription() << "Initializing feature...";
  ReadParameters();
  std::cerr << " Done." << std::endl;
}

void SourceGHKMTreeInputMatchFeature::SetParameter(const std::string& key, const std::string& value)
{
  UTIL_THROW(util::Exception, GetScoreProducerDescription() << ": Unknown parameter " << key << "=" << value);
}

// assumes that source-side syntax labels are stored in the target non-terminal field of the rules
void SourceGHKMTreeInputMatchFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  const WordsRange& wordsRange = inputPath.GetWordsRange();
  size_t startPos = wordsRange.GetStartPos();
  size_t endPos = wordsRange.GetEndPos();
  const TreeInput& treeInput = static_cast<const TreeInput&>(input);
  const NonTerminalSet& treeInputLabels = treeInput.GetLabelSet(startPos,endPos);
  const Word& lhsLabel = targetPhrase.GetTargetLHS();

  const StaticData& staticData = StaticData::Instance();
  const Word& outputDefaultNonTerminal = staticData.GetOutputDefaultNonTerminal();

  std::vector<float> newScores(m_numScoreComponents,0.0); // m_numScoreComponents == 2 // first fires for matches, second for mismatches

  if ( (treeInputLabels.find(lhsLabel) != treeInputLabels.end()) && (lhsLabel != outputDefaultNonTerminal) ) {
    // match
    newScores[0] = 1.0;
  } else {
    // mismatch
    newScores[1] = 1.0;
  }

  scoreBreakdown.PlusEquals(this, newScores);
}


}

