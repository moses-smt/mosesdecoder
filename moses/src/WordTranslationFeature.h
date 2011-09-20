#ifndef moses_WordTranslationFeature_h
#define moses_WordTranslationFeature_h

#include <string>
#include <map>

#include "FeatureFunction.h"
#include "FactorCollection.h"

namespace Moses
{

/** Sets the features for word translation
 */
class WordTranslationFeature : public StatelessFeatureFunction {
private:
  std::set<std::string> m_vocabSource;
  std::set<std::string> m_vocabTarget;
  FactorType m_factorTypeSource;
  FactorType m_factorTypeTarget;
  bool m_unrestricted;

public:
	WordTranslationFeature(FactorType factorTypeSource = 0, FactorType factorTypeTarget = 0):
     StatelessFeatureFunction("wt"),
     m_factorTypeSource(factorTypeSource),
     m_factorTypeTarget(factorTypeTarget),
     m_unrestricted(true)
  {}
      
	bool Load(const std::string &filePathSource, const std::string &filePathTarget);
  void Evaluate(const TargetPhrase& cur_phrase,
                ScoreComponentCollection* accumulator) const;

  // basic properties
	size_t GetNumScoreComponents() const { return ScoreProducer::unlimited; }
	std::string GetScoreProducerWeightShortName(unsigned) const { return "wt"; }
	size_t GetNumInputScores() const { return 0; }
};

}

#endif // moses_WordTranslationFeature_h
