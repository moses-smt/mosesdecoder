// $Id$

#ifndef moses_DynamicCacheBasedLanguageModel_h
#define moses_DynamicCacheBasedLanguageModel_h

#include "moses/Util.h"
#include "FeatureFunction.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

typedef std::pair<int, float> decaying_cache_value_t;
typedef std::map<std::string, decaying_cache_value_t > decaying_cache_t;

#define CBLM_QUERY_TYPE_UNDEFINED (-1)
#define CBLM_QUERY_TYPE_ALLSUBSTRINGS 0
#define CBLM_QUERY_TYPE_WHOLESTRING 1

#define CBLM_SCORE_TYPE_UNDEFINED (-1)
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

/** Calculates score for the Dynamic Cache-Based pseudo LM
 */
class DynamicCacheBasedLanguageModel : public StatelessFeatureFunction
{
  // data structure for the cache;
  // the key is the word and the value is the decaying score
  decaying_cache_t m_cache;
  size_t m_query_type; //way of querying the cache
  size_t m_score_type; //way of scoring entries of the cache
  std::string m_initfiles; // vector of files loaded in the initialization phase
  std::string m_name; // internal name to identify this instance of the Cache-based pseudo LM
  float m_lower_score; //lower_bound_score for no match
  bool m_constant; //flag for setting a non-decaying cache
  std::vector<float> precomputedScores;
  unsigned int m_maxAge;

#ifdef WITH_THREADS
  //multiple readers - single writer lock
  mutable boost::shared_mutex m_cacheLock;
#endif

  float decaying_score(unsigned int age);
  void SetPreComputedScores();
  float GetPreComputedScores(const unsigned int age);

  float Evaluate_Whole_String( const TargetPhrase&) const;
  float Evaluate_All_Substrings( const TargetPhrase&) const;

  void Decay();
  void Update(std::vector<std::string> words, int age);

  void ClearEntries(std::vector<std::string> entries);

  void Execute(std::vector<std::string> commands);
  void Execute_Single_Command(std::string command);

  void Load_Multiple_Files(std::vector<std::string> files);
  void Load_Single_File(const std::string file);

  void Insert(std::vector<std::string> ngrams);

//  void EvaluateInIsolation(const Phrase&, const TargetPhrase&, ScoreComponentCollection&, ScoreComponentCollection& ) const;
  void Print() const;

protected:
  static DynamicCacheBasedLanguageModel* s_instance;
  static std::map< const std::string, DynamicCacheBasedLanguageModel* > s_instance_map;

public:
  DynamicCacheBasedLanguageModel(const std::string &line);
  ~DynamicCacheBasedLanguageModel();

  inline const std::string GetName() {
    return m_name;
  };
  inline void SetName(const std::string name) {
    m_name = name;
  }

  static const DynamicCacheBasedLanguageModel* Instance(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }

  static DynamicCacheBasedLanguageModel* InstanceNonConst(const std::string& name) {
    if (s_instance_map.find(name) == s_instance_map.end()) {
      return NULL;
    }
    return s_instance_map[name];
  }



  static const DynamicCacheBasedLanguageModel& Instance() {
    return *s_instance;
  }
  static DynamicCacheBasedLanguageModel& InstanceNonConst() {
    return *s_instance;
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void Load();
  void Load(const std::string filestr);
  void Execute(std::string command);
  void SetParameter(const std::string& key, const std::string& value);
  void ExecuteDlt(std::map<std::string, std::string> dlt_meta);

  void ClearEntries(std::string &entries);
  void Insert(std::string &entries);
  void Clear();

  virtual void EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const;

  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const {
  }

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const {
  }

  void EvaluateWhenApplied(const Hypothesis& hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void EvaluateWhenApplied(const ChartHypothesis &hypo,
                           ScoreComponentCollection* accumulator) const {
  }

  void SetQueryType(size_t type);
  void SetScoreType(size_t type);
  void SetMaxAge(unsigned int age);
};

}

#endif
