// $Id$

#ifndef moses_CacheBasedLanguageModel_h
#define moses_CacheBasedLanguageModel_h

#include "FeatureFunction.h"

namespace Moses
{

class WordsRange;

/** Calculates Cache-based Language Model score
 */
class CacheBasedLanguageModel : public StatelessFeatureFunction
{
public:
  CacheBasedLanguageModel(ScoreIndexManager &scoreIndexManager);

  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumScoreComponents() const; 

  void Evaluate( const TargetPhrase&, ScoreComponentCollection* ) const;
}

};

#endif
