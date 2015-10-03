#pragma once

#include <string>
#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "moses/SearchNormal.h"
#include "moses/HypothesisStackNormal.h"
#include "moses/TranslationOptionCollection.h"
#include "StatefulFeatureFunction.h"
#include "FFState.h"

typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> CharAllocator;
typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> ShmemString;
typedef boost::interprocess::allocator<ShmemString, boost::interprocess::managed_shared_memory::segment_manager> ShmemStringAllocator;

typedef boost::interprocess::allocator<float, boost::interprocess::managed_shared_memory::segment_manager> ShmemFloatAllocator;
typedef boost::interprocess::allocator<void*, boost::interprocess::managed_shared_memory::segment_manager> ShmemVoidptrAllocator;
 
typedef std::vector<ShmemString, ShmemStringAllocator> ShmemStringVector;
typedef std::vector<float, ShmemFloatAllocator> ShmemFloatVector;
typedef std::vector<void*, ShmemVoidptrAllocator> ShmemVoidptrVector;

struct Payload {
  Payload() : state_(0), logProb_(0) {}
  Payload(void* state, float logProb) : state_(state), logProb_(logProb) {}
  
  void* state_;
  float logProb_;
};

typedef std::map<size_t, Payload> Payloads;
typedef std::vector<std::string> Prefix;
typedef std::map<Prefix, Payloads> Prefixes;
typedef std::vector<Prefixes> PrefsByLength;

typedef std::pair<const Prefix, Payloads> PP;
typedef std::pair<const size_t, Payload> SP;

namespace Moses
{

class NeuralScoreFeature : public StatefulFeatureFunction
{
public:
  NeuralScoreFeature(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

  void ProcessStack(Collector& collector, size_t index);
  
  /*
  void BatchProcess( const std::vector<std::string>& nextWords,
    PyObject* pyContextVectors,
    const std::vector< std::string >& lastWords,
    std::vector<PyObject*>& inputStates,
    std::vector<double>& logProbs,
    std::vector<PyObject*>& nextStates);
  */
  
  virtual const FFState* EmptyHypothesisState(const InputType &input) const;
  
  void EvaluateInIsolation(const Phrase &source
                           , const TargetPhrase &targetPhrase
                           , ScoreComponentCollection &scoreBreakdown
                           , ScoreComponentCollection &estimatedFutureScore) const;
  void EvaluateWithSourceContext(const InputType &input
                                 , const InputPath &inputPath
                                 , const TargetPhrase &targetPhrase
                                 , const StackVec *stackVec
                                 , ScoreComponentCollection &scoreBreakdown
                                 , ScoreComponentCollection *estimatedFutureScore = NULL) const;

  void EvaluateTranslationOptionListWithSourceContext(const InputType &input
      , const TranslationOptionList &translationOptionList) const;

  FFState* EvaluateWhenApplied(
    const Hypothesis& cur_hypo,
    const FFState* prev_state,
    ScoreComponentCollection* accumulator) const;
  FFState* EvaluateWhenApplied(
    const ChartHypothesis& /* cur_hypo */,
    int /* featureID - used to index the state in the previous hypotheses */,
    ScoreComponentCollection* accumulator) const;

  void SetParameter(const std::string& key, const std::string& value);
  
  void Init();

private:
  boost::shared_ptr<boost::interprocess::managed_shared_memory> m_segment;
  
  mutable boost::interprocess::named_mutex m_mutex;
  mutable boost::interprocess::named_condition m_moses;
  mutable boost::interprocess::named_condition m_neural;
  
  bool m_preCalc;
  std::string m_statePath;
  std::string m_modelPath;
  std::string m_wrapperPath;
  std::string m_sourceVocabPath;
  std::string m_targetVocabPath;
  //size_t m_batchSize;
  size_t m_stateLength;
  size_t m_factor;
    
  PrefsByLength m_pbl;
};

}

