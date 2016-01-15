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
#include "../Phrase.h"
#include "../legacy/InputFileStream.h"
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

  // cache
  CreateCache(system);
}

void ProbingPT::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
  RecycleData &recycler = GetThreadSpecificObj(m_recycleData);

  BOOST_FOREACH(InputPath *path, inputPaths) {
	TargetPhrases *tpsPtr;
	tpsPtr = Lookup(mgr, mgr.GetPool(), *path, recycler);
	path->AddTargetPhrases(*this, tpsPtr);
  }
}

TargetPhrases* ProbingPT::Lookup(const Manager &mgr,
		MemPool &pool,
		InputPath &inputPath,
		RecycleData &recycler) const
{
	/*
	if (inputPath.prefixPath && inputPath.prefixPath->GetTargetPhrases(*this) == NULL) {
		// assume all paths have prefixes, except rules with 1 word source
		return NULL;
	}
	else {
		const Phrase &sourcePhrase = inputPath.subPhrase;
		std::pair<TargetPhrases*, uint64_t> tpsAndKey = CreateTargetPhrase(pool, mgr.system, sourcePhrase, recycler);
		return tpsAndKey.first;
	}
	*/
	const Phrase &sourcePhrase = inputPath.subPhrase;
	CreateTargetPhraseStruct tpsAndKey = CreateTargetPhrase(pool, mgr.system, sourcePhrase, recycler);
	return tpsAndKey.tps;
}

ProbingPT::CreateTargetPhraseStruct ProbingPT::CreateTargetPhrase(
		  MemPool &pool,
		  const System &system,
		  const Phrase &sourcePhrase,
		  RecycleData &recycler) const
{
  CreateTargetPhraseStruct ret;
  ret.tps = NULL;

  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  size_t sourceSize = sourcePhrase.GetSize();
  assert(sourceSize);

  uint64_t probingSource[sourceSize];
  bool ok;
  ConvertToProbingSourcePhrase(sourcePhrase, ok, probingSource);
  if (!ok) {
    // source phrase contains a word unknown in the pt.
    // We know immediately there's no translation for it
    return ret;
  }

  //Actual lookup
  ret.key = m_engine->getKey(probingSource, sourceSize);

  std::pair<bool, std::vector<target_text*> > query_result;
  query_result = m_engine->query(ret.key, recycler);

  if (query_result.first) {
	//m_engine->printTargetInfo(query_result.second);
	const std::vector<target_text*> &probingTargetPhrases = query_result.second;
	ret.tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, probingTargetPhrases.size());

	for (size_t i = 0; i < probingTargetPhrases.size(); ++i) {
	  target_text *probingTargetPhrase = probingTargetPhrases[i];
	  TargetPhrase *tp = CreateTargetPhrase(pool, system, sourcePhrase, *probingTargetPhrase);

	  ret.tps->AddTargetPhrase(*tp);

	  recycler.tt.push_back(probingTargetPhrase);
	}

	ret.tps->SortAndPrune(m_tableLimit);
	system.featureFunctions.EvaluateAfterTablePruning(pool, *ret.tps, sourcePhrase);
  }
  else {
	  assert(query_result.second.size() == 0);
  }

  return ret;
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

  if (!m_engine->IsLogProb()) {
	  std::transform(scores, scores + probingTargetPhrase.prob.size(), scores, TransformScore);
	  std::transform(scores, scores + probingTargetPhrase.prob.size(), scores, FloorScore);
  }
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

void ProbingPT::CreateCache(System &system)
{
	if (m_maxCacheSize == 0) {
		return;
	}

	string filePath = m_path + "/cache";
	InputFileStream strme(filePath);

	string line;
	getline(strme, line);
	//float totalCount = Scan<float>(line);

	MemPool &pool = system.GetSystemPool();
	FactorCollection &vocab = system.GetVocab();
    RecycleData &recycler = GetThreadSpecificObj(m_recycleData); // prob should just create a temporary recycler

	size_t lineCount = 0;
	while (getline(strme, line) && lineCount < m_maxCacheSize) {
		vector<string> toks = Tokenize(line, "\t");
		assert(toks.size() == 2);
		PhraseImpl *sourcePhrase = PhraseImpl::CreateFromString(pool, vocab, system, toks[1]);

		CreateTargetPhraseStruct tpsAndKey = CreateTargetPhrase(pool, system, *sourcePhrase, recycler);
		assert(tpsAndKey.tps);

		//cerr << key << " " << *sourcePhrase << endl;
		m_cache[tpsAndKey.key] = tpsAndKey.tps;
	}


}

}

