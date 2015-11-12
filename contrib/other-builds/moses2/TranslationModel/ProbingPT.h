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

  TargetPhrases::shared_const_ptr Lookup(const Manager &mgr, InputPath &inputPath) const;

protected:
  typedef boost::bimap<const Factor *, uint64_t> SourceVocabMap;
  mutable SourceVocabMap m_sourceVocabMap;

  typedef boost::bimap<const Factor *, unsigned int> TargetVocabMap;
  mutable TargetVocabMap m_vocabMap;

  uint64_t m_unkId;
  QueryEngine *m_engine;

  TargetPhrases::shared_ptr CreateTargetPhrase(MemPool &pool, const System &system, const Phrase &sourcePhrase) const;
  TargetPhrase *CreateTargetPhrase(MemPool &pool, const System &system, const Phrase &sourcePhrase, const target_text &probingTargetPhrase) const;

  std::vector<uint64_t> ConvertToProbingSourcePhrase(const Phrase &sourcePhrase, bool &ok) const;

  const Factor *GetTargetFactor(uint64_t probingId) const;
  uint64_t GetSourceProbingId(const Factor *factor) const;

};

#endif /* FF_TRANSLATIONMODEL_PROBINGPT_H_ */
