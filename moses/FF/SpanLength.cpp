#include <boost/shared_ptr.hpp>
#include "SpanLength.h"
#include "moses/StaticData.h"
#include "moses/Word.h"
#include "moses/ChartCellLabel.h"
#include "moses/Range.h"
#include "moses/StackVec.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/PhraseProperty.h"
#include "moses/PP/SpanLengthPhraseProperty.h"

using namespace std;

namespace Moses
{
SpanLength::SpanLength(const std::string &line)
  :StatelessFeatureFunction(1, line)
  ,m_smoothingMethod(None)
  ,m_const(0)
{
  ReadParameters();
}

void SpanLength::EvaluateInIsolation(const Phrase &source
                                     , const TargetPhrase &targetPhrase
                                     , ScoreComponentCollection &scoreBreakdown
                                     , ScoreComponentCollection &estimatedScores) const
{
  targetPhrase.SetRuleSource(source);
}

void SpanLength::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedScores) const
{
  assert(stackVec);

  const PhraseProperty *property = targetPhrase.GetProperty("SpanLength");
  if (property == NULL) {
    return;
  }

  const SpanLengthPhraseProperty *slProp = static_cast<const SpanLengthPhraseProperty*>(property);

  assert(targetPhrase.GetRuleSource());

  float score = 0;
  for (size_t i = 0; i < stackVec->size(); ++i) {
    const ChartCellLabel &cell = *stackVec->at(i);
    const Range &ntRange = cell.GetCoverage();
    size_t sourceWidth = ntRange.GetNumWordsCovered();
    float prob = slProp->GetProb(i, sourceWidth, m_const);
    score += TransformScore(prob);
  }

  if (score < -100.0f) {
    float weight = StaticData::Instance().GetWeight(this);
    if (weight < 0) {
      score = -100;
    }
  }

  scoreBreakdown.PlusEquals(this, score);

}

void SpanLength::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "smoothing") {
    if (value == "plus-constant") {
      m_smoothingMethod = PlusConst;
    } else if (value == "none") {
      m_smoothingMethod = None;
    } else {
      UTIL_THROW(util::Exception, "Unknown smoothing type " << value);
    }
  } else if (key == "constant") {
    m_const = Scan<float>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

