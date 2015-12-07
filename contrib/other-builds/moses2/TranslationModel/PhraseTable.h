/*
 * PhraseTable.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "../TargetPhrases.h"
#include "../Word.h"
#include "../FF/StatelessFeatureFunction.h"
#include "../legacy/Util2.h"

class System;
class InputPaths;
class InputPath;
class Manager;

////////////////////////////////////////////////////////////////////////
class PhraseTable : public StatelessFeatureFunction
{
public:
	PhraseTable(size_t startInd, const std::string &line);
	virtual ~PhraseTable();

	virtual void SetParameter(const std::string& key, const std::string& value);
	virtual void Lookup(const Manager &mgr, InputPaths &inputPaths) const;
	virtual TargetPhrases::shared_const_ptr Lookup(const Manager &mgr, MemPool &pool, InputPath &inputPath) const;

	void SetPtInd(size_t ind)
	{ m_ptInd = ind; }
	size_t GetPtInd() const
	{ return m_ptInd; }

	  virtual void
	  EvaluateInIsolation(const System &system,
			  const Phrase &source, const TargetPhrase &targetPhrase,
			  Scores &scores,
			  Scores *estimatedScores) const;

	  virtual void CleanUpAfterSentenceProcessing();

protected:
  std::string m_path;
  size_t m_ptInd;
  size_t m_tableLimit;

  // cache
  size_t m_maxCacheSize; // 0 = no caching

  struct CacheCollEntry2
  {
	  TargetPhrases::shared_const_ptr tpsPtr;
	  clock_t clock;
  };

  typedef boost::unordered_map<size_t, CacheCollEntry2> CacheColl;
  mutable boost::thread_specific_ptr<CacheColl> m_cache;

  mutable boost::thread_specific_ptr<MemPool> m_cacheMemPool;


  CacheColl &GetCache() const;
  MemPool &GetCacheMemPool() const;

  void ReduceCache() const;

};

