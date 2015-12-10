/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <queue>
#include "PhraseTable.h"
#include "../InputPaths.h"
#include "../legacy/Util2.h"
#include "../TypeDef.h"
#include "../Search/Manager.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////
PhraseTable::PhraseTable(size_t startInd, const std::string &line)
:StatelessFeatureFunction(startInd, line)
,m_tableLimit(20) // default
,m_maxCacheSize(DEFAULT_MAX_TRANS_OPT_CACHE_SIZE)
{
	ReadParameters();
}

PhraseTable::~PhraseTable() {
	// TODO Auto-generated destructor stub
}

void PhraseTable::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "cache-size") {
    m_maxCacheSize = Scan<size_t>(value);
  }
  else if (key == "path") {
	  m_path = value;
  }
  else if (key == "input-factor") {

  }
  else if (key == "output-factor") {

  }
  else if (key == "table-limit") {
	  m_tableLimit = Scan<size_t>(value);
  }
  else {
	  StatelessFeatureFunction::SetParameter(key, value);
  }
}

void PhraseTable::Lookup(const Manager &mgr, InputPaths &inputPaths) const
{
  CacheColl &cache = GetCache();
  MemPool &pool = GetCacheMemPool();

  BOOST_FOREACH(InputPath &path, inputPaths) {
	  const SubPhrase &phrase = path.subPhrase;

	  TargetPhrases *tpsPtr = NULL;
	  if (m_maxCacheSize) {
		size_t hash = phrase.hash();

		CacheColl::iterator iter;
		iter = cache.find(hash);

		if (iter == cache.end()) {
		  // not in cache, need to look up from phrase table
		  tpsPtr = Lookup(mgr, pool, path);

		  CacheColl::value_type val(hash, CacheCollEntry2());
		  std::pair<CacheColl::iterator, bool> retIns = cache.insert(val);
		  assert(retIns.second);

		  CacheCollEntry2 &entry = retIns.first->second;
		  entry.clock = clock();
		  entry.tpsPtr = tpsPtr;

		}
		else {
		  // in cache. just use it
		  iter->second.clock = clock();
		  tpsPtr = iter->second.tpsPtr;
		}
	  } else {
		// don't use cache. look up from phrase table
		  tpsPtr = Lookup(mgr, mgr.GetPool(), path);
	  }

		/*
		cerr << "path=" << path.GetRange() << " ";
		cerr << "tps=" << tps << " ";
		if (tps.get()) {
			cerr << tps.get()->GetSize();
		}
		cerr << endl;
		*/

		path.AddTargetPhrases(*this, tpsPtr);
  }

}

TargetPhrases *PhraseTable::Lookup(const Manager &mgr, MemPool &pool, InputPath &inputPath) const
{
  UTIL_THROW2("Not implemented");
}

void
PhraseTable::EvaluateInIsolation(const System &system,
		const Phrase &source, const TargetPhrase &targetPhrase,
		Scores &scores,
		Scores *estimatedScores) const
{
}

void PhraseTable::CleanUpAfterSentenceProcessing()
{
	ReduceCache();
}

PhraseTable::CacheColl &PhraseTable::GetCache() const
{
  CacheColl &ret = GetThreadSpecificObj(m_cache);
  return ret;
}

MemPool &PhraseTable::GetCacheMemPool() const
{
  MemPool &ret = GetThreadSpecificObj(m_cacheMemPool);
  return ret;
}

// reduce presistent cache by half of maximum size
void PhraseTable::ReduceCache() const
{
  CacheColl &cache = GetCache();
  if (cache.size() <= m_maxCacheSize) return; // not full

  cerr << endl << "clearing cache" << endl;
  cache.clear();
  GetCacheMemPool().Reset();
  return;

  // find cutoff for last used time
  priority_queue< clock_t > lastUsedTimes;
  CacheColl::iterator iter;
  iter = cache.begin();
  while( iter != cache.end() ) {
    lastUsedTimes.push( iter->second.clock );
    iter++;
  }
  for( size_t i=0; i < lastUsedTimes.size()-m_maxCacheSize/2; i++ )
    lastUsedTimes.pop();
  clock_t cutoffLastUsedTime = lastUsedTimes.top();

  // remove all old entries
  iter = cache.begin();
  while( iter != cache.end() ) {
    if (iter->second.clock < cutoffLastUsedTime) {
      CacheColl::iterator iterRemove = iter++;
      // delete iterRemove->second.first;
      cache.erase(iterRemove);
    } else iter++;
  }
}

