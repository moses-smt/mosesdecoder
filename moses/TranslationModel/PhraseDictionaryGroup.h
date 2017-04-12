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

#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"
#include "moses/Util.h"

#include "moses/FF/LexicalReordering/LexicalReordering.h"

#include "moses/TranslationModel/PhraseDictionary.h"

#ifdef PT_UG
#include "moses/TranslationModel/UG/mmsapt.h"
#endif

namespace Moses
{

struct PDGroupPhrase {
  TargetPhrase* m_targetPhrase;
  std::vector<float> m_scores;
  boost::dynamic_bitset<> m_seenBy;

  PDGroupPhrase() : m_targetPhrase(NULL) { }

  PDGroupPhrase(TargetPhrase* targetPhrase, const std::vector<float>& scores, const size_t nModels)
    : m_targetPhrase(targetPhrase),
      m_scores(scores),
      m_seenBy(nModels) { }
};

/** Combines multiple phrase tables into a single interface.  Each member phrase
 * table scores each phrase and a single set of translations/scores is returned.
 * If a phrase is not in one of the tables, its scores are zero-filled unless
 * otherwise specified.  See model combination section of Moses advanced feature
 * documentation.
 */
class PhraseDictionaryGroup: public PhraseDictionary
{

public:
  PhraseDictionaryGroup(const std::string& line);
  void Load(AllOptions::ptr const& opts);
  TargetPhraseCollection::shared_ptr
  CreateTargetPhraseCollection(const ttasksptr& ttask,
                               const Phrase& src) const;
  std::vector<std::vector<float> > getWeights(size_t numWeights,
      bool normalize) const;
  void CacheForCleanup(TargetPhraseCollection::shared_ptr  tpc);
  void CleanUpAfterSentenceProcessing(const InputType& source);
  void CleanUpComponentModels(const InputType& source);
  // functions below override the base class
  void GetTargetPhraseCollectionBatch(const ttasksptr& ttask,
                                      const InputPathList &inputPathQueue) const;
  TargetPhraseCollection::shared_ptr  GetTargetPhraseCollectionLEGACY(
    const Phrase& src) const;
  TargetPhraseCollection::shared_ptr  GetTargetPhraseCollectionLEGACY(
    const ttasksptr& ttask, const Phrase& src) const;
  void InitializeForInput(ttasksptr const& ttask);
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&,
      const ChartCellCollectionBase&, std::size_t);
  void SetParameter(const std::string& key, const std::string& value);

protected:
  std::vector<std::string> m_memberPDStrs;
  std::vector<PhraseDictionary*> m_memberPDs;
  std::vector<FeatureFunction*> m_pdFeature;
  size_t m_numModels;
  size_t m_totalModelScores;
  boost::dynamic_bitset<> m_seenByAll;
  // phrase-counts option
  bool m_phraseCounts;
  // word-counts option
  bool m_wordCounts;
  // model-bitmap-counts option
  bool m_modelBitmapCounts;
  // restrict option
  bool m_restrict;
  // default-scores option
  bool m_haveDefaultScores;
  std::vector<float> m_defaultScores;
  // default-average-others option
  bool m_defaultAverageOthers;
  size_t m_scoresPerModel;
  // mmsapt-lr-func options
  bool m_haveMmsaptLrFunc;
  // pointers to pointers since member mmsapts may not load these until later
  std::vector<LexicalReordering**> m_mmsaptLrFuncs;

  typedef std::vector<TargetPhraseCollection::shared_ptr > PhraseCache;
#ifdef WITH_THREADS
  boost::shared_mutex m_lock_cache;
  typedef std::map<boost::thread::id, PhraseCache> SentenceCache;
#else
  typedef PhraseCache SentenceCache;
#endif
  SentenceCache m_sentenceCache;

  PhraseCache& GetPhraseCache() {
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
