/*
 * ProbingPT.h
 *
 *  Created on: 3 Nov 2015
 *      Author: hieu
 */

#ifndef FF_TRANSLATIONMODEL_PROBINGPT_H_
#define FF_TRANSLATIONMODEL_PROBINGPT_H_

#include <boost/bimap.hpp>
#include "PhraseTable.h"
#include "../Vector.h"

namespace Moses2
{

class Phrase;
class QueryEngine;
class target_text;
class MemPool;
class System;

class ProbingPT : public PhraseTable
{
public:
  ProbingPT(size_t startInd, const std::string &line);
  virtual ~ProbingPT();
  void Load(System &system);

  void Lookup(const Manager &mgr, InputPaths &inputPaths) const;

  virtual void GetScoresProperty(const std::string &key, size_t ind, SCORE *scoreArr);

protected:
  std::vector<uint64_t> m_sourceVocab; // factor id -> pt id
  std::vector<const Factor*> m_targetVocab; // pt id -> factor*

  uint64_t m_unkId;
  QueryEngine *m_engine;

  TargetPhrases *Lookup(const Manager &mgr,
		  MemPool &pool,
		  InputPath &inputPath,
		  std::pair<bool, std::vector<target_text*> > &query_result) const;
  TargetPhrases *CreateTargetPhrase(MemPool &pool,
		  const System &system,
		  const Phrase &sourcePhrase,
		  std::pair<bool, std::vector<target_text*> > &query_result) const;
  TargetPhrase *CreateTargetPhrase(MemPool &pool, const System &system,
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

  inline uint64_t GetSourceProbingId(const Factor *factor) const
  {
	  size_t factorId = factor->GetId();
	  if (factorId >= m_sourceVocab.size()) {
		  return m_unkId;
	  }
	  return m_sourceVocab[factorId];

  }

};

}

#endif /* FF_TRANSLATIONMODEL_PROBINGPT_H_ */
