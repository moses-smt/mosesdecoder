// $Id$

#ifndef moses_DynamicCacheBasedLanguageModel_h
#define moses_DynamicCacheBasedLanguageModel_h

#include "FeatureFunction.h"

typedef std::pair<int, float> decaying_cache_value_t; 
typedef std::map<std::string, decaying_cache_value_t > decaying_cache_t; 

#define ALLSUBSTRINGS 0
#define WHOLESTRING 1

namespace Moses
{

class WordsRange;

/** Calculates score for the Dynamic Cache-Based pesudo LM
 */
class DynamicCacheBasedLanguageModel : public StatelessFeatureFunction
{
  // data structure for the cache;
  // the key is the word and the value is the decaying score
  decaying_cache_t m_cache;
  size_t query_type; //way of querying the cache
  size_t score_type; //way of scoring entries of the cache
  std::string m_initfiles; // vector of files loaded in the initialization phase

  float decaying_score(int age);
  void Evaluate_Whole_String( const TargetPhrase&, ScoreComponentCollection* ) const;
  void Evaluate_All_Substrings( const TargetPhrase&, ScoreComponentCollection* ) const;

  void Decay();
  void Update(std::vector<std::string> words, int age);
  void Execute(std::string command);
  void Load(const std::string file);
	
  void Insert(std::vector<std::string> ngrams);

  void Print() const;
  void Clear();
  void Evaluate( const TargetPhrase&, ScoreComponentCollection* ) const;

public:
	
  DynamicCacheBasedLanguageModel(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Load();
  void SetParameter(const std::string& key, const std::string& value);

  inline void SetQueryType(size_t type) { query_type = type; };
  void Insert(std::string &entries);
  void Execute(std::vector<std::string> commands);
  void Load(std::vector<std::string> files);
  void Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
  void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;

};


}

#endif
