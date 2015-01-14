#include <vector>
#include "PhrasePenalty.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
PhrasePenalty::PhrasePenalty(const std::string &line)
  : StatelessFeatureFunction(1, line)
  , m_perPhraseTable(false)
{
  ReadParameters();
}

void PhrasePenalty::EvaluateInIsolation(const Phrase &source
                                        , const TargetPhrase &targetPhrase
                                        , ScoreComponentCollection &scoreBreakdown
                                        , ScoreComponentCollection &estimatedFutureScore) const
{
  if (m_perPhraseTable) {
    const PhraseDictionary *pt = targetPhrase.GetContainer();
    if (pt) {
      size_t ptId = pt->GetId();
      UTIL_THROW_IF2(ptId >= m_numScoreComponents, "Wrong number of scores");

      vector<float> scores(m_numScoreComponents, 0);
      scores[ptId] = 1.0f;

      scoreBreakdown.Assign(this, scores);
    }

  } else {
    scoreBreakdown.Assign(this, 1.0f);
  }
}

void PhrasePenalty::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "per-phrase-table") {
    m_perPhraseTable =Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


} // namespace

