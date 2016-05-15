/*
 * GPULM.cpp
 *
 *  Created on: 4 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include <vector>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "GPULM.h"
#include "../Phrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Manager.h"
#include "util/exception.hh"
#include "../legacy/FactorCollection.h"
#include <boost/thread/tss.hpp>

using namespace std;

namespace Moses2
{

struct GPULMState: public FFState
{
  virtual std::string ToString() const
  {
    return "GPULMState";
  }

  virtual size_t hash() const
  {
    return boost::hash_value(lastWords);
  }

  virtual bool operator==(const FFState& other) const
  {
    const GPULMState &otherCast = static_cast<const GPULMState&>(other);
    bool ret = lastWords == otherCast.lastWords;

    return ret;
  }

  void SetContext(const Context &context)
  {
    lastWords = context;
    if (lastWords.size()) {
      lastWords.resize(lastWords.size() - 1);
    }
  }

  Context lastWords;
};


/////////////////////////////////////////////////////////////////
GPULM::GPULM(size_t startInd, const std::string &line)
:StatefulFeatureFunction(startInd, line)
{
  cerr << "GPULM::GPULM" << endl;
  m_threadID.store(1);
  ReadParameters();
}

GPULM::~GPULM()
{
  // Free pinned memory:
  //@TODO FREE PINNED MEMORY HERE
  //pinnedMemoryDeallocator(ngrams_for_query);
  //pinnedMemoryDeallocator(results);
}

QueryMemory * GPULM::getThreadQueryMemory() const {
    QueryMemory * res = m_QueryMemory.get();
    if(!res) {
        QueryMemory * local_querymem = new QueryMemory(m_threadID, max_num_queries, max_ngram_order);
        //pinnedMemoryAllocator(local_results, max_num_queries); //Max max_num_queries ngram batches @TODO NO FREE
        m_QueryMemory.reset(local_querymem);
        res = local_querymem;
    }
    return res;
}

void GPULM::Load(System &system)
{
  int deviceID = 0; //@TODO This is an optional argument
  max_num_queries = 100000;
  m_obj = new gpuLM(m_path, max_num_queries, deviceID);
  cerr << "GPULM::Load" << endl;
  
  //Allocate host memory here. Size should be same as the constructor
  max_ngram_order = m_obj->getMaxNumNgrams();
  m_order = max_ngram_order;
  
  //Add factors 
  FactorCollection &vocab = system.GetVocab();
  std::unordered_map<std::string, unsigned int>& origmap = m_obj->getEncodeMap();

  for (auto it : origmap) {
    const Factor *factor = vocab.AddFactor(it.first, system, false);
    encode_map.insert(std::make_pair(factor, it.second));
  }
  
}

void GPULM::SetParameter(const std::string& key,
    const std::string& value)
{
  //cerr << "key=" << key << " " << value << endl;
  if (key == "path") {
    m_path = value;
  }
  else if (key == "order") {
    m_order = Scan<size_t>(value);
  }
  else if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  }
  else {
    StatefulFeatureFunction::SetParameter(key, value);
  }

  //cerr << "SetParameter done" << endl;
}

FFState* GPULM::BlankState(MemPool &pool) const
{
  GPULMState *ret = new (pool.Allocate<GPULMState>()) GPULMState();
  return ret;
}

//! return the state associated with the empty hypothesis for a given sentence
void GPULM::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
    const InputType &input, const Hypothesis &hypo) const
{
  GPULMState &stateCast = static_cast<GPULMState&>(state);
  stateCast.lastWords.push_back(m_bos);
}

void GPULM::EvaluateInIsolation(MemPool &pool, const System &system,
    const Phrase<Moses2::Word> &source, const TargetPhrase<Moses2::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{
  if (targetPhrase.GetSize() == 0) {
    return;
  }

  SCORE score = 0;
  SCORE nonFullScore = 0;
  Context context;
//  context.push_back(m_bos);

  context.reserve(m_order);
  QueryMemory * query_mem = getThreadQueryMemory();
  unsigned int * ngrams_for_query = query_mem->ngrams_for_query;
  float * results = query_mem->results;
  //std::cerr << "Size: " << targetPhrase.GetSize()<< std::endl;
  for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
    const Factor *factor = targetPhrase[i][m_factorType];
    ShiftOrPush(context, factor);

    unsigned int position = 0; //Position in ngrams_for_query array
    unsigned int num_queries = 1;
    CreateQueryVec(context, position);
    m_obj->query(results, ngrams_for_query, num_queries); //@TODO do this

    if (context.size() == m_order) {
      SCORE score = results[position];
      //std::pair<SCORE, void*> fromScoring = Score(context);
      score += score;
    }
    else if (estimatedScore) {
      SCORE score = results[position];
      //std::pair<SCORE, void*> fromScoring = Score(context);
      nonFullScore += score;
    }
  }

}

void GPULM::EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
    const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
    SCORE *estimatedScore) const
{
  UTIL_THROW2("Not implemented");
}

void GPULM::EvaluateWhenApplied(const ManagerBase &mgr,
    const Hypothesis &hypo, const FFState &prevState, Scores &scores,
    FFState &state) const
{
  UTIL_THROW2("Not implemented");
}

void GPULM::EvaluateWhenAppliedBatch(
    const System &system,
    const Batch &batch) const
{
  // create list of ngrams
  std::vector<std::pair<Hypothesis*, Context> > contexts;

  for (size_t i = 0; i < batch.size(); ++i) {
    Hypothesis *hypo = batch[i];
    CreateNGram(contexts, *hypo);
  }
  
  QueryMemory * query_mem = getThreadQueryMemory();
  unsigned int * ngrams_for_query = query_mem->ngrams_for_query;
  float * results = query_mem->results;
  
  //Create the query vector
  unsigned int position = 0; //Position in ngrams_for_query array
  unsigned int num_queries = 0;
  for (auto context : contexts) { //@TODO can this be 0? need to handle this case
    num_queries++;
    CreateQueryVec(context.second, position);
  }

  //Score here + copy back-and-forth
  m_obj->query(results, ngrams_for_query, num_queries);

  // score ngrams
  for (size_t i = 0; i < contexts.size(); ++i) {
    const Context &context = contexts[i].second;
    Hypothesis *hypo = contexts[i].first;
    SCORE score = results[i];
    Scores &scores = hypo->GetScores();
    scores.PlusEquals(system, *this, score);
  }
}

void GPULM::CreateQueryVec(
		  const Context &context,
		  unsigned int &position) const
{
    QueryMemory * query_mem = getThreadQueryMemory();
    unsigned int * ngrams_for_query = query_mem->ngrams_for_query;
    int counter = 0; //Check for non full ngrams

    for (auto factor : context) {
      auto vocabID = encode_map.find(factor);
      if (vocabID == encode_map.end()){
        ngrams_for_query[position] = 1; //UNK
      } else {
        ngrams_for_query[position] = vocabID->second;
      }
      counter++;
      position++;
    }
    while (counter < max_ngram_order) {
      ngrams_for_query[position] = 0;
      counter++;
      position++;
    }
    
    if (position > max_num_queries*max_ngram_order) {
      std::cerr << "Number of queries exceeded the allocated space! Please increase the max_num_queries" << std::endl;
    }

}

void GPULM::CreateNGram(std::vector<std::pair<Hypothesis*, Context> > &contexts, Hypothesis &hypo) const
{
  const TargetPhrase<Moses2::Word> &tp = hypo.GetTargetPhrase();

  if (tp.GetSize() == 0) {
    return;
  }

  const Hypothesis *prevHypo = hypo.GetPrevHypo();
  assert(prevHypo);
  const FFState *prevState = prevHypo->GetState(GetStatefulInd());
  assert(prevState);
  const GPULMState &prevStateCast = static_cast<const GPULMState&>(*prevState);

  Context context = prevStateCast.lastWords;
  context.reserve(m_order);

  for (size_t i = 0; i < tp.GetSize(); ++i) {
    const Word &word = tp[i];
    const Factor *factor = word[m_factorType];
    ShiftOrPush(context, factor);

    std::pair<Hypothesis*, Context> ele(&hypo, context);
    contexts.push_back(ele);
  }

  FFState *state = hypo.GetState(GetStatefulInd());
  GPULMState &stateCast = static_cast<GPULMState&>(*state);
  stateCast.SetContext(context);
}

void GPULM::ShiftOrPush(std::vector<const Factor*> &context,
    const Factor *factor) const
{
  if (context.size() < m_order) {
    context.resize(context.size() + 1);
  }
  assert(context.size());

  for (size_t i = context.size() - 1; i > 0; --i) {
    context[i] = context[i - 1];
  }

  context[0] = factor;
}

}

