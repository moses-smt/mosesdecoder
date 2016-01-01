/*
 * ProbingPT.cpp
 *
 *  Created on: 3 Nov 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "ProbingPT.h"
#include "../System.h"
#include "../Scores.h"
#include "../FF/FeatureFunctions.h"
#include "../Search/Manager.h"
#include "../legacy/FactorCollection.h"
#include "../legacy/ProbingPT/quering.hh"
#include "../legacy/Util2.h"

using namespace std;

namespace Moses2
{

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
	size_t factorId = factor->GetId();

	if (factorId >= m_sourceVocab.size()) {
		m_sourceVocab.resize(factorId + 1, m_unkId);
	}
	m_sourceVocab[factorId] = probingId;
  }

  // target vocab
  const std::map<unsigned int, std::string> &probingVocab = m_engine->getVocab();
  std::map<unsigned int, std::string>::const_iterator iter;
  for (iter = probingVocab.begin(); iter != probingVocab.end(); ++iter) {
	const string &wordStr = iter->second;
	const Factor *factor = vocab.AddFactor(wordStr, system);

	unsigned int probingId = iter->first;

	if (probingId >= m_targetVocab.size()) {
		m_targetVocab.resize(probingId + 1, NULL);
	}
	m_targetVocab[probingId] = factor;
  }
}

void ProbingPT::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
  BOOST_FOREACH(InputPath &path, inputPaths) {
	  const SubPhrase &phrase = path.subPhrase;

	TargetPhrases *tpsPtr;
	tpsPtr = Lookup(mgr, mgr.GetPool(), path);
	path.AddTargetPhrases(*this, tpsPtr);
  }

}

TargetPhrases* ProbingPT::Lookup(const Manager &mgr, MemPool &pool, InputPath &inputPath) const
{
	const Phrase &sourcePhrase = inputPath.subPhrase;
	TargetPhrases *ret = CreateTargetPhrase(pool, mgr.system, sourcePhrase);
	return ret;
}

TargetPhrases* ProbingPT::CreateTargetPhrase(MemPool &pool, const System &system, const Phrase &sourcePhrase) const
{

  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  size_t sourceSize = sourcePhrase.GetSize();
	assert(sourceSize);

  TargetPhrases *tps = NULL;
  uint64_t probingSource[sourceSize];
  bool ok;
  ConvertToProbingSourcePhrase(sourcePhrase, ok, probingSource);
  if (!ok) {
    // source phrase contains a word unknown in the pt.
    // We know immediately there's no translation for it
    return tps;
  }

  std::pair<bool, std::vector<target_text*> > query_result;

  //Actual lookup
  query_result = m_engine->query(probingSource, sourceSize);

  if (query_result.first) {
    //m_engine->printTargetInfo(query_result.second);
	const std::vector<target_text*> &probingTargetPhrases = query_result.second;
	tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, probingTargetPhrases.size());

    for (size_t i = 0; i < probingTargetPhrases.size(); ++i) {
      const target_text *probingTargetPhrase = probingTargetPhrases[i];
      TargetPhrase *tp = CreateTargetPhrase(pool, system, sourcePhrase, *probingTargetPhrase);

      tps->AddTargetPhrase(*tp);

      delete probingTargetPhrase;
    }

    tps->SortAndPrune(m_tableLimit);
    system.featureFunctions.EvaluateAfterTablePruning(pool, *tps, sourcePhrase);
  }
  else {
	  assert(query_result.second.size() == 0);
  }

  return tps;

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
  SCORE scores[probingTargetPhrase.prob.size()];
  std::copy(probingTargetPhrase.prob.begin(), probingTargetPhrase.prob.end(), scores);
  std::transform(scores, scores + probingTargetPhrase.prob.size(), scores, TransformScore);
  std::transform(scores, scores + probingTargetPhrase.prob.size(), scores, FloorScore);
  tp->GetScores().PlusEquals(system, *this, scores);

  // extra scores
  //cerr << "probingTargetPhrase.prob.size()=" << probingTargetPhrase.prob.size() << endl;
  if (probingTargetPhrase.prob.size() > m_numScores) {
	  // we have extra scores, possibly for lex ro. Keep them in the target phrase.
	  size_t numExtraScores = probingTargetPhrase.prob.size() - m_numScores;
	  tp->scoreProperties = pool.Allocate<SCORE>(numExtraScores);
	  memcpy(tp->scoreProperties, scores + m_numScores, sizeof(SCORE) * numExtraScores);

	  /*
	  for (size_t i = 0; i < probingTargetPhrase.prob.size(); ++i) {
		  cerr << probingTargetPhrase.prob[i] << " ";
	  }
	  cerr << endl;

	  for (size_t i = 0; i < probingTargetPhrase.prob.size(); ++i) {
		  cerr << scores[i] << " ";
	  }
	  cerr << endl;

	  for (size_t i = 0; i < numExtraScores; ++i) {
		  cerr << tp->scoreProperties[i] << " ";
	  }
	  cerr << endl;
	  */
  }

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

void ProbingPT::ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok, uint64_t probingSource[]) const
{

  size_t size = sourcePhrase.GetSize();
  for (size_t i = 0; i < size; ++i) {
    const Factor *factor = sourcePhrase[i][0];
    uint64_t probingId = GetSourceProbingId(factor);
    if (probingId == m_unkId) {
      ok = false;
      return;
    } else {
      probingSource[i] = probingId;
    }
  }

  ok = true;
}

void ProbingPT::GetScoresProperty(const std::string &key, size_t ind, SCORE *scoreArr)
		{

		}

}

