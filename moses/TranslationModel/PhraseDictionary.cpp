// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <queue>
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/StaticData.h"
#include "moses/InputType.h"
#include "moses/TranslationOption.h"
#include "moses/DecodeStep.h"
#include "moses/DecodeGraph.h"
#include "moses/InputPath.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
std::vector<PhraseDictionary*> PhraseDictionary::s_staticColl;

CacheColl::~CacheColl()
{
	for (iterator iter = begin(); iter != end(); ++iter) {
		std::pair<const TargetPhraseCollection*, clock_t> &key = iter->second;
		const TargetPhraseCollection *tps = key.first;
		delete tps;
	}
}

PhraseDictionary::PhraseDictionary(const std::string &line)
  :DecodeFeature(line)
  ,m_tableLimit(20) // default
  ,m_maxCacheSize(DEFAULT_MAX_TRANS_OPT_CACHE_SIZE)
{
	m_id = s_staticColl.size();
	s_staticColl.push_back(this);
}

bool
PhraseDictionary::
ProvidesPrefixCheck() const
{
  return false;
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollectionLEGACY(const Phrase& src) const
{
  const TargetPhraseCollection *ret;
  if (m_maxCacheSize) {
    CacheColl &cache = GetCache();

    size_t hash = hash_value(src);

    CacheColl::iterator iter;
    iter = cache.find(hash);

    if (iter == cache.end()) {
      // not in cache, need to look up from phrase table
      ret = GetTargetPhraseCollectionNonCacheLEGACY(src);
      if (ret) {
        ret = new TargetPhraseCollection(*ret);
      }

      std::pair<const TargetPhraseCollection*, clock_t> value(ret, clock());
      cache[hash] = value;
    } else {
      // in cache. just use it
      std::pair<const TargetPhraseCollection*, clock_t> &value = iter->second;
      value.second = clock();

      ret = value.first;
    }
  } else {
    // don't use cache. look up from phrase table
    ret = GetTargetPhraseCollectionNonCacheLEGACY(src);
  }

  return ret;
}

TargetPhraseCollection const *
PhraseDictionary::
GetTargetPhraseCollectionNonCacheLEGACY(const Phrase& src) const
{
  UTIL_THROW(util::Exception, "Legacy method not implemented");
}


TargetPhraseCollectionWithSourcePhrase const*
PhraseDictionary::
GetTargetPhraseCollectionLEGACY(InputType const& src,WordsRange const& range) const
{
  UTIL_THROW(util::Exception, "Legacy method not implemented");
}

void
PhraseDictionary::
SetParameter(const std::string& key, const std::string& value)
{
  if (key == "cache-size") {
    m_maxCacheSize = Scan<size_t>(value);
  } else if (key == "path") {
    m_filePath = value;
  } else if (key == "table-limit") {
    m_tableLimit = Scan<size_t>(value);
  } else {
    DecodeFeature::SetParameter(key, value);
  }
}

void
PhraseDictionary::
SetFeaturesToApply()
{
  // find out which feature function can be applied in this decode step
  const std::vector<FeatureFunction*> &allFeatures = FeatureFunction::GetFeatureFunctions();
  for (size_t i = 0; i < allFeatures.size(); ++i) {
    FeatureFunction *feature = allFeatures[i];
    if (feature->IsUseable(m_outputFactors)) {
      m_featuresToApply.push_back(feature);
    }
  }
}

  
  // tell the Phrase Dictionary that the TargetPhraseCollection is not needed any more
  void
  PhraseDictionary::
  Release(TargetPhraseCollection const* tpc) const
  {
    // do nothing by default
    return;
  }

  bool
  PhraseDictionary::
  PrefixExists(Phrase const& phrase) const
  {
    return true;
  }

void
PhraseDictionary::
GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &inputPath = **iter;

    // backoff
    if (!SatisfyBackoff(inputPath)) {
    	continue;
    }

    const Phrase &phrase = inputPath.GetPhrase();
    const TargetPhraseCollection *targetPhrases = this->GetTargetPhraseCollectionLEGACY(phrase);
    inputPath.SetTargetPhrases(*this, targetPhrases, NULL);
  }
}

// persistent cache handling
// saving presistent cache to disk
//void PhraseDictionary::SaveCache() const
//{
//  CacheColl &cache = GetCache();
//  for( std::map<size_t, std::pair<const TargetPhraseCollection*,clock_t> >::iterator iter,
//       iter != cache.end(),
//       iter++ ) {
//    
//  }
//}

// loading persistent cache from disk
//void PhraseDictionary::LoadCache() const
//{
//  CacheColl &cache = GetCache();
//  std::map<size_t, std::pair<const TargetPhraseCollection*,clock_t> >::iterator iter;
//  iter = cache.begin();
//  while( iter != cache.end() ) {
//    std::map<size_t, std::pair<const TargetPhraseCollection*,clock_t> >::iterator iterRemove = iter++;
//    delete iterRemove->second.first;
//    cache.erase(iterRemove);
//  }
//}

// reduce presistent cache by half of maximum size
void PhraseDictionary::ReduceCache() const
{
  Timer reduceCacheTime;
  reduceCacheTime.start();
  CacheColl &cache = GetCache();
  if (cache.size() <= m_maxCacheSize) return; // not full

  // find cutoff for last used time
  priority_queue< clock_t > lastUsedTimes;
  CacheColl::iterator iter;
  iter = cache.begin();
  while( iter != cache.end() ) {
    lastUsedTimes.push( iter->second.second );
    iter++;
  }
  for( size_t i=0; i < lastUsedTimes.size()-m_maxCacheSize/2; i++ )
    lastUsedTimes.pop();
  clock_t cutoffLastUsedTime = lastUsedTimes.top();

  // remove all old entries
  iter = cache.begin();
  while( iter != cache.end() ) {
    if (iter->second.second < cutoffLastUsedTime) {
      CacheColl::iterator iterRemove = iter++;
      delete iterRemove->second.first;
      cache.erase(iterRemove);
    } else iter++;
  }
  VERBOSE(2,"Reduced persistent translation option cache in " << reduceCacheTime << " seconds." << std::endl);
}

CacheColl &PhraseDictionary::GetCache() const
{
  CacheColl *cache;
  cache = m_cache.get();
  if (cache == NULL) {
    cache = new CacheColl;
    m_cache.reset(cache);
  }
  assert(cache);
  return *cache;
}

bool PhraseDictionary::SatisfyBackoff(const InputPath &inputPath) const
{
  const Phrase &sourcePhrase = inputPath.GetPhrase();

  assert(m_container);
  const DecodeGraph &decodeGraph = GetDecodeGraph();
  size_t backoff = decodeGraph.GetBackoff();

  if (backoff == 0) {
	  // ie. don't backoff. Collect ALL translations
	  return true;
  }

  if (sourcePhrase.GetSize() > backoff) {
	  // source phrase too big
	  return false;
  }

  // lookup translation only if no other translations
  InputPath::TargetPhrases::const_iterator iter;
  for (iter = inputPath.GetTargetPhrases().begin(); iter != inputPath.GetTargetPhrases().end(); ++iter) {
  	const std::pair<const TargetPhraseCollection*, const void*> &temp = iter->second;
  	const TargetPhraseCollection *tpCollPrev = temp.first;

  	if (tpCollPrev && tpCollPrev->GetSize()) {
  		// already have translation from another pt. Don't create translations
  		return false;
  	}
  }

  return true;
}

} // namespace

