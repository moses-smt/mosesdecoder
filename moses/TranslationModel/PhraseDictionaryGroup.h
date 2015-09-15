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

#ifndef moses_PhraseDictionaryGroup_h
#define moses_PhraseDictionaryGroup_h

#include "moses/TranslationModel/PhraseDictionary.h"

#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/Util.h"

namespace Moses
{

/** Combines multiple phrase tables into a single interface.  Each member phrase
 * table scores each phrase and a single set of translations/scores is returned.
 * If a phrase is not in one of the tables, its scores are zero-filled.  Use the
 * "restrict" option to restrict phrases to those in the table-limit of the
 * first member table, intended to be a "union" table built on all data.
 */
class PhraseDictionaryGroup: public PhraseDictionary {

public:
  PhraseDictionaryGroup(const std::string& line);
  void Load();
  TargetPhraseCollection* CreateTargetPhraseCollection(const ttasksptr& ttask,
      const Phrase& src) const;
  std::vector<std::vector<float> > getWeights(size_t numWeights,
      bool normalize) const;
  void CacheForCleanup(TargetPhraseCollection* tpc);
  void CleanUpAfterSentenceProcessing(const InputType& source);
  void CleanUpComponentModels(const InputType& source);
  // functions below override the base class
  void GetTargetPhraseCollectionBatch(const ttasksptr& ttask,
      const InputPathList &inputPathQueue) const;
  const TargetPhraseCollection* GetTargetPhraseCollectionLEGACY(
      const Phrase& src) const;
  const TargetPhraseCollection* GetTargetPhraseCollectionLEGACY(
      const ttasksptr& ttask, const Phrase& src) const;
  void InitializeForInput(ttasksptr const& ttask)
  {
    /* Don't do anything source specific here as this object is shared between threads.*/
  }
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&,
      const ChartCellCollectionBase&, std::size_t);
  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::vector<std::string> m_memberPDStrs;
  std::vector<PhraseDictionary*> m_memberPDs;
  size_t m_numModels;
  bool m_restrict;
  std::vector<FeatureFunction*> m_pdFeature;

  typedef std::vector<TargetPhraseCollection*> PhraseCache;
#ifdef WITH_THREADS
  boost::shared_mutex m_lock_cache;
  typedef std::map<boost::thread::id, PhraseCache> SentenceCache;
#else
  typedef PhraseCache SentenceCache;
#endif
  SentenceCache m_sentenceCache;

  PhraseCache& GetPhraseCache()
  {
#ifdef WITH_THREADS
    {
      // first try read-only lock
      boost::shared_lock<boost::shared_mutex> read_lock(m_lock_cache);
      SentenceCache::iterator i = m_sentenceCache.find(
          boost::this_thread::get_id());
      if (i != m_sentenceCache.end())
        return i->second;
    }
    boost::unique_lock<boost::shared_mutex> lock(m_lock_cache);
    return m_sentenceCache[boost::this_thread::get_id()];
#else
    return m_sentenceCache;
#endif
  }
};

} // end namespace

#endif
