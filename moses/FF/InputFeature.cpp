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
InputFeature *InputFeature::s_instance = NULL;

InputFeature::InputFeature(const std::string &line)
  : StatelessFeatureFunction(line)
  , m_numRealWordCount(0)
{
  m_numInputScores = this->m_numScoreComponents;
  ReadParameters();

  UTIL_THROW_IF2(s_instance, "Can only have 1 input feature");
  s_instance = this;
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

void InputFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
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

