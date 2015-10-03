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

namespace bi = boost::interprocess;

typedef bi::allocator<char, bi::managed_shared_memory::segment_manager> CharAllocator;
typedef bi::basic_string<char, std::char_traits<char>, CharAllocator> ShmemString;
typedef bi::allocator<ShmemString, bi::managed_shared_memory::segment_manager> ShmemStringAllocator;

typedef bi::allocator<float, bi::managed_shared_memory::segment_manager> ShmemFloatAllocator;
typedef bi::allocator<void*, bi::managed_shared_memory::segment_manager> ShmemVoidptrAllocator;
 
typedef std::vector<ShmemString, ShmemStringAllocator> ShmemStringVector;
typedef std::vector<float, ShmemFloatAllocator> ShmemFloatVector;
typedef std::vector<void*, ShmemVoidptrAllocator> ShmemVoidptrVector;

struct Calculator {
  public:
    Calculator(const std::string& statePath, const std::string& modelPath,
               const std::string& wrapperPath, const std::string& sourceVocab,
               const std::string& targetVocab) {
      
      bi::shared_memory_object::remove("NeuralSharedMemory");
      m_segment.reset(new bi::managed_shared_memory(bi::create_only,
                                                    "NeuralSharedMemory",
                                                    1024 * 1024 * 500));
      
      // construc mutex and conditions
      m_segment->construct<bi::interprocess_mutex>("Mutex")();
      m_segment->construct<bi::interprocess_condition>("ParentCondition")();
      m_segment->construct<bi::interprocess_condition>("ChildCondition")();
    
      const CharAllocator charAlloc(m_segment->get_segment_manager());
      const ShmemStringAllocator stringAlloc(m_segment->get_segment_manager());
      const ShmemFloatAllocator floatAlloc(m_segment->get_segment_manager());
      const ShmemVoidptrAllocator voidptrAlloc(m_segment->get_segment_manager());
      
      *m_segment->construct<ShmemString>("StatePath")(charAlloc) = statePath.c_str();
      *m_segment->construct<ShmemString>("ModelPath")(charAlloc) = modelPath.c_str();
      *m_segment->construct<ShmemString>("WrapperPath")(charAlloc) = wrapperPath.c_str();
      *m_segment->construct<ShmemString>("SourceVocab")(charAlloc) = sourceVocab.c_str();
      *m_segment->construct<ShmemString>("TargetVocab")(charAlloc) = targetVocab.c_str();
      
      m_segment->construct<bool>("SentenceIsDone")(false);
      
      allWords = m_segment->construct<ShmemStringVector>("AllWords")(stringAlloc);
      allLastWords = m_segment->construct<ShmemStringVector>("AllLastWords")(stringAlloc);
      allStates = m_segment->construct<ShmemVoidptrVector>("AllStates")(voidptrAlloc);
      allOutProbs = m_segment->construct<ShmemFloatVector>("AllOutProbs")(floatAlloc);
      allOutStates = m_segment->construct<ShmemVoidptrVector>("AllOutStates")(voidptrAlloc);
      
      WaitForChild();
    }

    
    ~Calculator() {
      NotifyChild();
      bi::shared_memory_object::remove("NeuralSharedMemory");
    }
    
    void SentenceDone(bool status) {
      *m_segment->find<bool>("SentenceIsDone").first = status;
    }
    
    void NotifyChild() {
      m_segment->find<bi::interprocess_condition>("ChildCondition").first->notify_one();
    }
    
    void WaitForChild() {
      bi::interprocess_mutex* mutex
        = m_segment->find<bi::interprocess_mutex>("Mutex").first;
      bi::scoped_lock<bi::interprocess_mutex> lock(*mutex);
      
      bi::interprocess_condition* parent
        = m_segment->find<bi::interprocess_condition>("ParentCondition").first;
      
      parent->wait(lock);
    }
    
    void NotifyChildAndWait() {
      bi::interprocess_mutex* mutex
        = m_segment->find<bi::interprocess_mutex>("Mutex").first;
      bi::scoped_lock<bi::interprocess_mutex> lock(*mutex);  
      m_segment->find<bi::interprocess_condition>("ChildCondition").first->notify_one();      
      bi::interprocess_condition* parent
        = m_segment->find<bi::interprocess_condition>("ParentCondition").first;
      parent->wait(lock);
    }
    
    void* GetEmptyHypothesisState(const std::string& sentence) {
      SentenceDone(false);
      void** state = m_segment->construct<void*>("ContextPtr")((void*)0);            
      
      CharAllocator charAlloc(m_segment->get_segment_manager());
      ShmemString* shSentence = m_segment->construct<ShmemString>("ContextString")(charAlloc);
      *shSentence = sentence.c_str();
      
      NotifyChildAndWait();
      
      m_segment->destroy_ptr(shSentence);
      return *state;
    }
    
    void AddWord(const std::string& word) {
      const CharAllocator charAlloc(m_segment->get_segment_manager());
      ShmemString tempWord(charAlloc);
      tempWord = word.c_str();
      allWords->push_back(tempWord);
    }
    
    void AddLastWord(const std::string& word) {
      const CharAllocator charAlloc(m_segment->get_segment_manager());
      ShmemString tempWord(charAlloc);
      tempWord = word.c_str();
      allLastWords->push_back(tempWord);      
    }
    
    void AddState(void* state) {
      allStates->push_back(state);  
    }
    
    float GetProb(size_t k) {
      return (*allOutProbs)[k];
    }
    
    void* GetState(size_t k) {
      return (*allOutStates)[k];
    }
    
    void Clear() {
      allWords->clear();
      allLastWords->clear();
      allStates->clear();
      allOutProbs->clear();
      allOutStates->clear();
    }
    
    size_t GetSize() {
      return allStates->size();
    }
    
  private:
    boost::shared_ptr<bi::managed_shared_memory> m_segment;
    ShmemStringVector *allWords;
    ShmemStringVector *allLastWords;
    ShmemVoidptrVector *allStates;
    ShmemFloatVector *allOutProbs;
    ShmemVoidptrVector *allOutStates;   
};

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
  boost::shared_ptr<Calculator> m_calculator;
  
  bool m_preCalc;
  std::string m_statePath;
  std::string m_modelPath;
  std::string m_wrapperPath;
  std::string m_sourceVocabPath;
  std::string m_targetVocabPath;
  
  size_t m_stateLength;
  size_t m_factor;
    
  PrefsByLength m_pbl;
};

}

