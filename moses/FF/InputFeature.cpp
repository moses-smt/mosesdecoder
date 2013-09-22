#include <stdexcept>
#include "InputFeature.h"
#include "moses/Util.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/InputPath.h"
#include "moses/StaticData.h"
#include "moses/TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "util/check.hh"

using namespace std;

namespace Moses
{
InputFeature::InputFeature(const std::string &line)
  :StatelessFeatureFunction("InputFeature", line)
{
  ReadParameters();
}

void InputFeature::Load()
{
	const StaticData &staticData = StaticData::Instance();
	const PhraseDictionary *pt = staticData.GetTranslationScoreProducer(0);
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
                            , ScoreComponentCollection &scoreBreakdown) const
{
	if (m_legacy) {
		//binary phrase-table does input feature itself
		return;
	}

  const ScorePair *scores = inputPath.GetInputScore();
  if (scores) {

  }
}

} // namespace

