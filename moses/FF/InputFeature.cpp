#include <stdexcept>
#include "InputFeature.h"
#include "moses/Util.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/InputPath.h"
#include "moses/StaticData.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"

using namespace std;

namespace Moses
{
InputFeature::InputFeature(const std::string &line)
  :StatelessFeatureFunction(line)
{
  ReadParameters();
}

void InputFeature::Load()
{
  const PhraseDictionary *pt = PhraseDictionary::GetColl()[0];
  const PhraseDictionaryTreeAdaptor *ptBin = dynamic_cast<const PhraseDictionaryTreeAdaptor*>(pt);

  m_legacy = (ptBin != NULL);
}

void InputFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "num-input-features") {
    m_numInputScores = Scan<size_t>(value);
  } else if (key == "real-word-count") {
    m_numRealWordCount = Scan<size_t>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }

}

void InputFeature::Evaluate(const InputType &input
                            , const InputPath &inputPath
                            , const TargetPhrase &targetPhrase
                            , ScoreComponentCollection &scoreBreakdown
                            , ScoreComponentCollection *estimatedFutureScore) const
{
  if (m_legacy) {
    //binary phrase-table does input feature itself
    return;
  }
  /*
  const ScorePair *scores = inputPath.GetInputScore();
  if (scores) {
  	  scoreBreakdown.PlusEquals(this, *scores);
  }
  */
}

} // namespace

