/*
 * ProbingPT.cpp
 *
 *  Created on: 3 Nov 2015
 *      Author: hieu
 */

#include "ProbingPT.h"
#include "../System.h"
#include "../Scores.h"
#include "../FF/FeatureFunctions.h"
#include "../Search/Manager.h"
#include "../legacy/FactorCollection.h"
#include "../legacy/ProbingPT/quering.hh"
#include "../legacy/Util2.h"

using namespace std;

ProbingPT::ProbingPT(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
	  ReadParameters();
}

ProbingPT::~ProbingPT()
{
  delete m_engine;
}

void ProbingPT::Load(System &system)
{
  m_engine = new QueryEngine(m_path.c_str());

  m_unkId = 456456546456;

  FactorCollection &vocab = system.GetVocab();

  // source vocab
  const std::map<uint64_t, std::string> &sourceVocab = m_engine->getSourceVocab();
  std::map<uint64_t, std::string>::const_iterator iterSource;
  for (iterSource = sourceVocab.begin(); iterSource != sourceVocab.end(); ++iterSource) {
	const string &wordStr = iterSource->second;
	const Factor *factor = vocab.AddFactor(wordStr, system);

	uint64_t probingId = iterSource->first;

	SourceVocabMap::value_type entry(factor, probingId);
	m_sourceVocabMap.insert(entry);

  }

  // target vocab
  const std::map<unsigned int, std::string> &probingVocab = m_engine->getVocab();
  std::map<unsigned int, std::string>::const_iterator iter;
  for (iter = probingVocab.begin(); iter != probingVocab.end(); ++iter) {
	const string &wordStr = iter->second;
	const Factor *factor = vocab.AddFactor(wordStr, system);

	unsigned int probingId = iter->first;

	TargetVocabMap::value_type entry(factor, probingId);
	m_vocabMap.insert(entry);

  }
}


const Factor *ProbingPT::GetTargetFactor(uint64_t probingId) const
{
  TargetVocabMap::right_map::const_iterator iter;
  iter = m_vocabMap.right.find(probingId);
  if (iter != m_vocabMap.right.end()) {
    return iter->second;
  } else {
    // not in mapping. Must be UNK
    return NULL;
  }
}

uint64_t ProbingPT::GetSourceProbingId(const Factor *factor) const
{
  SourceVocabMap::left_map::const_iterator iter;
  iter = m_sourceVocabMap.left.find(factor);
  if (iter != m_sourceVocabMap.left.end()) {
    return iter->second;
  } else {
    // not in mapping. Must be UNK
    return m_unkId;
  }
}

TargetPhrases::shared_const_ptr ProbingPT::Lookup(const Manager &mgr, InputPath &inputPath) const
{
	const Phrase &sourcePhrase = inputPath.subPhrase;
	TargetPhrases::shared_const_ptr ret = CreateTargetPhrase(mgr.GetPool(), mgr.system, sourcePhrase);
	return ret;
}

TargetPhrases::shared_ptr ProbingPT::CreateTargetPhrase(MemPool &pool, const System &system, const Phrase &sourcePhrase) const
{

  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  assert(sourcePhrase.GetSize());

  TargetPhrases::shared_ptr tpSharedPtr;
  bool ok;
  vector<uint64_t> probingSource = ConvertToProbingSourcePhrase(sourcePhrase, ok);
  if (!ok) {
    // source phrase contains a word unknown in the pt.
    // We know immediately there's no translation for it
    return tpSharedPtr;
  }

  std::pair<bool, std::vector<target_text> > query_result;

  //Actual lookup
  query_result = m_engine->query(probingSource);

  if (query_result.first) {
    //m_engine->printTargetInfo(query_result.second);
    tpSharedPtr.reset(new TargetPhrases());

    const std::vector<target_text> &probingTargetPhrases = query_result.second;
    for (size_t i = 0; i < probingTargetPhrases.size(); ++i) {
      const target_text &probingTargetPhrase = probingTargetPhrases[i];
      TargetPhrase *tp = CreateTargetPhrase(pool, system, sourcePhrase, probingTargetPhrase);

      tpSharedPtr->AddTargetPhrase(*tp);
    }

    tpSharedPtr->SortAndPrune(m_tableLimit);
  }

  return tpSharedPtr;

}

TargetPhrase *ProbingPT::CreateTargetPhrase(MemPool &pool, const System &system, const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const
{

  const std::vector<unsigned int> &probingPhrase = probingTargetPhrase.target_phrase;
  size_t size = probingPhrase.size();

  TargetPhrase *tp = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, system, size);

  // words
  for (size_t i = 0; i < size; ++i) {
    uint64_t probingId = probingPhrase[i];
    const Factor *factor = GetTargetFactor(probingId);
    assert(factor);

    Word &word = (*tp)[i];
    word[0] = factor;
  }

  // score for this phrase table
  vector<SCORE> scores = probingTargetPhrase.prob;
  std::transform(scores.begin(), scores.end(), scores.begin(), TransformScore);
  tp->GetScores().PlusEquals(system, *this, scores);

//  // alignment
//  /*
//  const std::vector<unsigned char> &alignments = probingTargetPhrase.word_all1;
//
//  AlignmentInfo &aligns = tp->GetAlignTerm();
//  for (size_t i = 0; i < alignS.size(); i += 2 ) {
//    aligns.Add((size_t) alignments[i], (size_t) alignments[i+1]);
//  }
//  */

  // score of all other ff when this rule is being loaded
  const FeatureFunctions &ffs = system.featureFunctions;
  ffs.EvaluateInIsolation(pool, system, sourcePhrase, *tp);
  return tp;

}

std::vector<uint64_t> ProbingPT::ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok) const
{

  size_t size = sourcePhrase.GetSize();
  std::vector<uint64_t> ret(size);
  for (size_t i = 0; i < size; ++i) {
    const Factor *factor = sourcePhrase[i][0];
    uint64_t probingId = GetSourceProbingId(factor);
    if (probingId == m_unkId) {
      ok = false;
      return ret;
    } else {
      ret[i] = probingId;
    }
  }

  ok = true;
  return ret;

}

