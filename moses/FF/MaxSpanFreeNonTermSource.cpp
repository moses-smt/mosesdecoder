#include <iostream>
#include "MaxSpanFreeNonTermSource.h"
#include "moses/StaticData.h"
#include "moses/Word.h"
#include "moses/InputPath.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"
#include "moses/WordsRange.h"
#include "moses/ChartCellLabel.h"
#include "moses/FactorCollection.h"

using namespace std;

namespace Moses
{
MaxSpanFreeNonTermSource::MaxSpanFreeNonTermSource(const std::string &line)
  :StatelessFeatureFunction(1, line)
  ,m_maxSpan(2)
  ,m_glueTargetLHSStr("S")
  ,m_glueTargetLHS(true)
{
  m_tuneable = false;
  ReadParameters();

  FactorCollection &fc = FactorCollection::Instance();
  const Factor *factor = fc.AddFactor(m_glueTargetLHSStr, true);
  m_glueTargetLHS.SetFactor(0, factor);
}

void MaxSpanFreeNonTermSource::EvaluateInIsolation(const Phrase &source
    , const TargetPhrase &targetPhrase
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void MaxSpanFreeNonTermSource::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  const Word &targetLHS = targetPhrase.GetTargetLHS();

  if (targetLHS == m_glueTargetLHS) {
    // don't delete glue rules
    return;
  }

  const Phrase *source = targetPhrase.GetRuleSource();
  assert(source);
  float score = 0;

  if (source->Front().IsNonTerminal()) {
    const ChartCellLabel &cell = *stackVec->front();
    if (cell.GetCoverage().GetNumWordsCovered() > m_maxSpan) {
      score = - std::numeric_limits<float>::infinity();
    }
  }

  if (source->Back().IsNonTerminal()) {
    const ChartCellLabel &cell = *stackVec->back();
    if (cell.GetCoverage().GetNumWordsCovered() > m_maxSpan) {
      score = - std::numeric_limits<float>::infinity();
    }
  }


  scoreBreakdown.PlusEquals(this, score);

}


void MaxSpanFreeNonTermSource::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "max-span") {
    m_maxSpan = Scan<int>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

std::vector<float> MaxSpanFreeNonTermSource::DefaultWeights() const
{
  std::vector<float> ret(1, 1);
  return ret;
}

}

