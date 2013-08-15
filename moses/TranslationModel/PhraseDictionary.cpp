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

PhraseDictionary::PhraseDictionary(const std::string &description, const std::string &line)
  :DecodeFeature(description, line)
  ,m_tableLimit(20) // default
  ,m_useCache(666)
{
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollection(const Phrase& src) const
{
  const TargetPhraseCollection *ret;
  if (m_useCache) {
    size_t hash = hash_value(src);

    std::map<size_t, const TargetPhraseCollection*>::const_iterator iter;

    {
      // scope of read lock
#ifdef WITH_THREADS
      boost::shared_lock<boost::shared_mutex> read_lock(m_accessLock);
#endif
      iter = m_cache.find(hash);
    }

    if (iter == m_cache.end()) {
      ret = GetTargetPhraseCollectionNonCache(src);
      if (ret) {
        ret = new TargetPhraseCollection(*ret);
      }

#ifdef WITH_THREADS
      boost::unique_lock<boost::shared_mutex> lock(m_accessLock);
#endif
      m_cache[hash] = ret;
    } else {
      ret = iter->second;
    }
  } else {
    ret = GetTargetPhraseCollectionNonCache(src);
  }

  return ret;
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollectionNonCache(const Phrase& src) const
{
  UTIL_THROW(util::Exception, "Legacy method not implemented");
}


const TargetPhraseCollectionWithSourcePhrase* PhraseDictionary::
GetTargetPhraseCollectionLegacy(InputType const& src,WordsRange const& range) const
{
  UTIL_THROW(util::Exception, "Legacy method not implemented");
  //Phrase phrase = src.GetSubString(range);
  //return GetTargetPhraseCollection(phrase);
}

void PhraseDictionary::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "use-cache") {
    m_useCache = Scan<int>(value);
  } else if (key == "path") {
    m_filePath = value;
  } else if (key == "table-limit") {
    m_tableLimit = Scan<size_t>(value);
  } else {
    DecodeFeature::SetParameter(key, value);
  }
}

void PhraseDictionary::SetFeaturesToApply()
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

void PhraseDictionary::GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const
{
  InputPathList::const_iterator iter;
  for (iter = phraseDictionaryQueue.begin(); iter != phraseDictionaryQueue.end(); ++iter) {
    InputPath &node = **iter;

    const Phrase &phrase = node.GetPhrase();
    const TargetPhraseCollection *targetPhrases = this->GetTargetPhraseCollection(phrase);
    node.SetTargetPhrases(*this, targetPhrases, NULL);
  }
}

}

