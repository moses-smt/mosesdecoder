// $Id$

#ifndef moses_PSDScoreProducer_h
#define moses_PSDScoreProducer_h

#include "FeatureFunction.h"
#include "vw.h"
#include "TypeDef.h"
#include "TranslationOption.h"
#include "ScoreComponentCollection.h"
#include "InputType.h"
#include <map>
#include <string>
#include <vector>

namespace Moses
{

const size_t CONTEXT_SIZE = 2;
typedef std::map<std::string, size_t> PhraseIndexType;

struct VWInstance
{
  ::vw m_vw;
};

extern VWInstance vwInstance;

class PSDScoreProducer : public StatelessFeatureFunction
{
public:
  PSDScoreProducer(ScoreIndexManager &scoreIndexManager, float weight);

  // read required data files
  bool Initialize(const std::string &modelFile, const std::string &indexFile);

  // score a list of translation options
  // this is required to contain all possible translations
  // of a given source span
  vector<ScoreComponentCollection> ScoreOptions(
    const vector<TranslationOption *> &options,
    const InputType &source);

  // mandatory methods for features
  size_t GetNumScoreComponents() const;
  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumInputScores() const;

  virtual bool ComputeValueInTranslationOption() const
  {
    return true;
  }
private:
  bool LoadPhraseIndex(const string &indexFile);
  bool IsOOV(const TargetPhrase &tgtPhrase);
  ScoreComponentCollection ScoreFactory(float score);
  vector<string> GetSourceFeatures(const InputType &srcSentence, const TranslationOption *tOpt);
  vector<string> GetTargetFeatures(const TargetPhrase &tgtPhrase);

  std::vector<FactorType> m_srcFactors, m_tgtFactors; // which factors to use; XXX hard-coded for now
  PhraseIndexType m_phraseIndex;
};

}

#endif // moses_PSDScoreProducer_h
