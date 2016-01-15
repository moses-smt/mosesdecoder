/*
 * ProbingPT.h
 *
 *  Created on: 3 Nov 2015
 *      Author: hieu
 */

#ifndef FF_TRANSLATIONMODEL_PROBINGPT_H_
#define FF_TRANSLATIONMODEL_PROBINGPT_H_

#include <boost/thread/tss.hpp>
#include <boost/bimap.hpp>
#include <deque>
#include "PhraseTable.h"
#include "../Vector.h"

namespace Moses2
{

class Phrase;
class QueryEngine;
class target_text;
class MemPool;
class System;
class RecycleData;

class ProbingPT : public PhraseTable
{
  struct RetStruct
  {
	  bool keyOK;
	  uint64_t key;
  };

public:
  ProbingPT(size_t startInd, const std::string &line);
  virtual ~ProbingPT();
  void Load(System &system);

  void Lookup(const Manager &mgr, InputPaths &inputPaths) const;

protected:
  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*

  uint64_t m_unkId;
  QueryEngine *m_engine;

  mutable boost::thread_specific_ptr< std::deque<target_text*> > m_recycler;
  mutable boost::thread_specific_ptr<RecycleData> m_recycleData;

  TargetPhrases *Lookup(const Manager &mgr,
		  MemPool &pool,
		  InputPath &inputPath,
		  RecycleData &recycler) const;
  TargetPhrases *CreateTargetPhrase(MemPool &pool,
		  const System &system,
		  const Phrase &sourcePhrase,
		  RecycleData &recycler,
		  RetStruct &retStruct) const;
  TargetPhrase *CreateTargetPhrase(MemPool &pool,
		  const System &system,
		  const Phrase &sourcePhrase,
		  const target_text &probingTargetPhrase) const;

  void ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok, uint64_t probingSource[]) const;

  inline const Factor *GetTargetFactor(uint64_t probingId) const
  {
	  if (probingId >= m_targetVocab.size()) {
		  return NULL;
	  }
	  return m_targetVocab[probingId];
  }

  void GetSourceProbingId(const Phrase &sourcePhrase, RetStruct &retStruct) const;
  inline uint64_t GetSourceProbingId(const Factor *factor) const
  {
	  size_t factorId = factor->GetId();
	  if (factorId >= m_sourceVocab.size()) {
		  return m_unkId;
	  }
	  return m_sourceVocab[factorId];

  }

  // caching
  boost::unordered_map<uint64_t, TargetPhrases*> m_cache;
  void CreateCache(System &system);

};

}

#endif /* FF_TRANSLATIONMODEL_PROBINGPT_H_ */
