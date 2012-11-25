// $Id$

#ifndef moses_CacheBasedLanguageModel_h
#define moses_CacheBasedLanguageModel_h

#include "FeatureFunction.h"
#include "InputFileStream.h"

typedef std::pair<int, float> decaying_cache_value_t; 
typedef std::map<std::string, decaying_cache_value_t > decaying_cache_t; 

#define ALLSUBSTRING 0
#define WHOLESTRING 1

namespace Moses
{

class WordsRange;

/** Calculates Cache-based Language Model score
 */
class CacheBasedLanguageModel : public StatelessFeatureFunction
{
// data structure for the cache;
// the key is the word and the value is the decaying score
  decaying_cache_t m_cache;
  size_t query_type; //way of querying the cache

  float decaying_score(int age);
  void Evaluate_Whole_String( const TargetPhrase&, ScoreComponentCollection* ) const;
  void Evaluate_All_Substrings( const TargetPhrase&, ScoreComponentCollection* ) const;

public:
  CacheBasedLanguageModel(const std::vector<float>& weights);

  std::string GetScoreProducerDescription(unsigned) const;
  std::string GetScoreProducerWeightShortName(unsigned) const;
  size_t GetNumScoreComponents() const; 
  void SetQueryType(size_t type);

  void Evaluate( const TargetPhrase&, ScoreComponentCollection* ) const;

  void Decay();
  void Update(std::vector<std::string> words, int age);
  void Insert(std::vector<std::string> words);
  void Load(const std::string file);

  void PrintCache();
};

}

#endif
