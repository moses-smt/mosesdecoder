// $Id$

#ifndef moses_CacheBasedLanguageModel_h
#define moses_CacheBasedLanguageModel_h

#include "FeatureFunction.h"
#include "InputFileStream.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

typedef std::pair<int, float> decaying_cache_value_t; 
typedef std::map<std::string, decaying_cache_value_t > decaying_cache_t; 

#define CBLM_QUERY_TYPE_ALLSUBSTRINGS 0
#define CBLM_QUERY_TYPE_WHOLESTRING 1

#define CBLM_SCORE_TYPE_HYPERBOLA 0
#define CBLM_SCORE_TYPE_POWER 1
#define CBLM_SCORE_TYPE_EXPONENTIAL 2
#define CBLM_SCORE_TYPE_COSINE 3
#define CBLM_SCORE_TYPE_HYPERBOLA_REWARD 10
#define CBLM_SCORE_TYPE_POWER_REWARD 11
#define CBLM_SCORE_TYPE_EXPONENTIAL_REWARD 12
#define PI 3.14159265

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
  std::vector<float> precomputedScores;
  unsigned int maxAge;

  size_t query_type; //way of querying the cache
  size_t score_type; //scoring type of the match
#ifdef WITH_THREADS
  //multiple readers - single writer lock
  mutable boost::shared_mutex m_cacheLock;
#endif

  float decaying_score(int age);
  void SetPreComputedScores();

  void Evaluate_Whole_String( const TargetPhrase&, ScoreComponentCollection* ) const;
  void Evaluate_All_Substrings( const TargetPhrase&, ScoreComponentCollection* ) const;

  void Decay();
  void Update(std::vector<std::string> words, int age);
  void Execute(std::string command);
  void Load(const std::string file);
	
  void Print() const;
  void Clear();
  void Evaluate( const TargetPhrase&, ScoreComponentCollection* ) const;

  void SetQueryType(size_t type);
  void SetScoreType(size_t type);
  void SetMaxAge(unsigned int age);


public:
  CacheBasedLanguageModel(const std::vector<std::string>& files, const size_t q_type, const size_t s_type, const unsigned int age);

  inline size_t GetNumScoreComponents() const { return 1; };
  inline std::string GetScoreProducerWeightShortName(unsigned) const { return "cblm"; };
	
  void Insert(std::vector<std::string> ngrams);
  void Execute(std::vector<std::string> commands);
  void Load(std::vector<std::string> files);
  void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
  void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
};


}

#endif
