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

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/StaticData.h"
#include "moses/InputType.h"
#include "moses/TranslationOption.h"
#include "moses/UserMessage.h"
#include "moses/InputPath.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{
std::vector<PhraseDictionary*> PhraseDictionary::s_staticColl;

PhraseDictionary::PhraseDictionary(const std::string &line)
  :DecodeFeature(line)
  ,m_tableLimit(20) // default
  ,m_maxCacheSize(DEFAULT_MAX_TRANS_OPT_CACHE_SIZE)
{
	s_staticColl.push_back(this);
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollectionLEGACY(const Phrase& src) const
{
  const TargetPhraseCollection *ret;
  if (m_maxCacheSize) {
    CacheColl &cache = GetCache();

    size_t hash = hash_value(src);

    std::map<size_t, std::pair<const TargetPhraseCollection*, clock_t> >::iterator iter;

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

void
PhraseDictionary::
GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = inputPathQueue.begin(); iter != inputPathQueue.end(); ++iter) {
    InputPath &node = **iter;

    const Phrase &phrase = node.GetPhrase();
    const TargetPhraseCollection *targetPhrases = this->GetTargetPhraseCollectionLEGACY(phrase);
    node.SetTargetPhrases(*this, targetPhrases, NULL);
  }
}

void PhraseDictionary::ReduceCache() const
{
  CacheColl &cache = GetCache();
  if (cache.size() <= m_maxCacheSize) return; // not full

  // find cutoff for last used time
  priority_queue< clock_t > lastUsedTimes;
  std::map<size_t, std::pair<const TargetPhraseCollection*,clock_t> >::iterator iter;
  iter = cache.begin();
  while( iter != cache.end() ) {
    lastUsedTimes.push( iter->second.second );
    iter++;
  }
  for( size_t i=0; i < lastUsedTimes.size()-m_maxCacheSize/2; i++ )
    lastUsedTimes.pop();
  clock_t cutoffLastUsedTime = lastUsedTimes.top();
  clock_t t = clock();

  // remove all old entries
  iter = cache.begin();
  while( iter != cache.end() ) {
    if (iter->second.second < cutoffLastUsedTime) {
      std::map<size_t, std::pair<const TargetPhraseCollection*,clock_t> >::iterator iterRemove = iter++;
      delete iterRemove->second.first;
      cache.erase(iterRemove);
    } else iter++;
  }
  VERBOSE(2,"Reduced persistent translation option cache in " << ((clock()-t)/(float)CLOCKS_PER_SEC) << " seconds." << std::endl);
}

PhraseDictionary::CacheColl &PhraseDictionary::GetCache() const
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

} // namespace

