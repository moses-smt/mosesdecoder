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
#include "../legacy/ProbingPT/probing_hash_utils.hh"
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
  InputFileStream targetVocabStrme(m_path + "/TargetVocab.dat");
  string line;
  while (getline(targetVocabStrme, line)) {
	  vector<string> toks = Tokenize(line, "\t");
	  assert(toks.size());
  	  const Factor *factor = vocab.AddFactor(toks[0], system);
  	  uint32_t probingId = Scan<uint32_t>(toks[1]);

  	  if (probingId >= m_targetVocab.size()) {
  		m_targetVocab.resize(probingId + 1, NULL);
  	  }
  	  m_targetVocab[probingId] = factor;
  }

  // memory mapped file to tps
  string filePath = m_path + "/TargetColl.dat";
  file.open(filePath.c_str());
  if (!file.is_open()) {
    throw "Couldn't open file ";
  }

  data = file.data();

  size_t size = file.size();

  // cache
  CreateCache(system);
}

void ProbingPT::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
  BOOST_FOREACH(InputPath *path, inputPaths) {
	TargetPhrases *tpsPtr;
	tpsPtr = Lookup(mgr, mgr.GetPool(), *path);
	path->AddTargetPhrases(*this, tpsPtr);
  }
}

TargetPhrases* ProbingPT::Lookup(const Manager &mgr,
		MemPool &pool,
		InputPath &inputPath) const
{
	/*
	if (inputPath.prefixPath && inputPath.prefixPath->GetTargetPhrases(*this) == NULL) {
		// assume all paths have prefixes, except rules with 1 word source
		return NULL;
	}
	else {
		const Phrase &sourcePhrase = inputPath.subPhrase;
		std::pair<TargetPhrases*, uint64_t> tpsAndKey = CreateTargetPhrase(pool, mgr.system, sourcePhrase);
		return tpsAndKey.first;
	}
	*/
	const Phrase &sourcePhrase = inputPath.subPhrase;

	// get hash for source phrase
	std::pair<bool, uint64_t> keyStruct = GetSourceProbingId(sourcePhrase);
	if (!keyStruct.first) {
	  return NULL;
	}

	// check in cache
	Cache::const_iterator iter = m_cache.find(keyStruct.second);
	if (iter != m_cache.end()) {
    	    cerr << "cache 1\n";
		TargetPhrases *tps = iter->second;
		return tps;
	}
        else {
          cerr << "cache 0\n";
        }

	// query pt
	TargetPhrases *tps = CreateTargetPhrase(pool, mgr.system, sourcePhrase, keyStruct.second);
	return tps;
}

std::pair<bool, uint64_t> ProbingPT::GetSourceProbingId(const Phrase &sourcePhrase) const
{
  std::pair<bool, uint64_t> ret;

  // create a target phrase from the 1st word of the source, prefix with 'ProbingPT:'
  size_t sourceSize = sourcePhrase.GetSize();
  assert(sourceSize);

  uint64_t probingSource[sourceSize];
  ConvertToProbingSourcePhrase(sourcePhrase, ret.first, probingSource);
  if (!ret.first) {
	// source phrase contains a word unknown in the pt.
	// We know immediately there's no translation for it
  }
  else {
	  ret.second = m_engine->getKey(probingSource, sourceSize);
  }

  return ret;

}

TargetPhrases *ProbingPT::CreateTargetPhrase(
		  MemPool &pool,
		  const System &system,
		  const Phrase &sourcePhrase,
		  uint64_t key) const
{
  TargetPhrases *tps = NULL;

  //Actual lookup
  std::pair<bool, uint64_t> query_result; // 1st=found, 2nd=target file offset
  query_result = m_engine->query(key);

  if (query_result.first) {
	  const char *offset = data + query_result.second;
	  uint64_t *numTP = (uint64_t*) offset;

	  tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, *numTP);

	  offset += sizeof(uint64_t);
	  for (size_t i = 0; i < *numTP; ++i) {
		  TargetPhrase *tp = CreateTargetPhrase(pool, system, offset);
		  assert(tp);
		  const FeatureFunctions &ffs = system.featureFunctions;
		  ffs.EvaluateInIsolation(pool, system, sourcePhrase, *tp);

		  tps->AddTargetPhrase(*tp);

	  }

	  tps->SortAndPrune(m_tableLimit);
	  system.featureFunctions.EvaluateAfterTablePruning(pool, *tps, sourcePhrase);
	  //cerr << *tps << endl;
  }

  return tps;
}

TargetPhrase *ProbingPT::CreateTargetPhrase(
		  MemPool &pool,
		  const System &system,
		  const char *&offset) const
{
	TargetPhraseInfo *tpInfo = (TargetPhraseInfo*) offset;
    TargetPhrase *tp = new (pool.Allocate<TargetPhrase>()) TargetPhrase(pool, *this, system, tpInfo->numWords);

	offset += sizeof(TargetPhraseInfo);

	// scores
	SCORE *scores = (SCORE*) offset;

  size_t totalNumScores = m_engine->num_scores + m_engine->num_lex_scores;

  if (m_engine->logProb) {
	    // set pt score for rule
	    tp->GetScores().PlusEquals(system, *this, scores);

	    // save scores for other FF, eg. lex RO. Just give the offset
	    if (m_engine->num_lex_scores) {
		  tp->scoreProperties = scores + m_engine->num_scores;
	    }
  }
  else {
	  // log score 1st
	  SCORE logScores[totalNumScores];
	  for (size_t i = 0; i < totalNumScores; ++i) {
		  logScores[i] = FloorScore(TransformScore(scores[i]));
	  }

      // set pt score for rule
	  tp->GetScores().PlusEquals(system, *this, logScores);

      // save scores for other FF, eg. lex RO.
	  tp->scoreProperties = pool.Allocate<SCORE>(m_engine->num_lex_scores);
	  for (size_t i = 0; i < m_engine->num_lex_scores; ++i) {
		  tp->scoreProperties[i] = logScores[i + m_engine->num_scores];
	  }
  }

  offset += sizeof(SCORE) * totalNumScores;

  // words
  for (size_t i = 0; i < tpInfo->numWords; ++i) {
	  uint32_t *probingId = (uint32_t*) offset;

	  const Factor *factor = GetTargetFactor(*probingId);
	  assert(factor);

	  Word &word = (*tp)[i];
	  word[0] = factor;

	  offset += sizeof(uint32_t);
  }

  // properties TODO

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

	size_t lineCount = 0;
	while (getline(strme, line) && lineCount < m_maxCacheSize) {
		vector<string> toks = Tokenize(line, "\t");
		assert(toks.size() == 2);
		PhraseImpl *sourcePhrase = PhraseImpl::CreateFromString(pool, vocab, system, toks[1]);

		std::pair<bool, uint64_t> retStruct = GetSourceProbingId(*sourcePhrase);
		if (!retStruct.first) {
		  return;
		}

		TargetPhrases *tps = CreateTargetPhrase(pool, system, *sourcePhrase, retStruct.second);
		assert(tps);

		m_cache[retStruct.second] = tps;

		++lineCount;
	}


}

}

