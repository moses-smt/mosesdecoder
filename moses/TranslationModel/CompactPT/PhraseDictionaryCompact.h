// $Id$
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

#ifndef moses_PhraseDictionaryCompact_h
#define moses_PhraseDictionaryCompact_h

#include <boost/unordered_map.hpp>

#ifdef WITH_THREADS
#ifdef BOOST_HAS_PTHREADS
#include <boost/thread/mutex.hpp>
#endif
#endif

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/ThreadPool.h"

#include "BlockHashIndex.h"
#include "StringVector.h"
#include "PhraseDecoder.h"
#include "TargetPhraseCollectionCache.h"

namespace Moses
{

class PhraseDecoder;

class PhraseDictionaryCompact : public PhraseDictionary
{
protected:
  friend class PhraseDecoder;

  static bool s_inMemoryByDefault;
  bool m_inMemory;
  bool m_useAlignmentInfo;

  typedef std::vector<TargetPhraseCollection::shared_ptr > PhraseCache;
  typedef boost::thread_specific_ptr<PhraseCache> SentenceCache;
  static SentenceCache m_sentenceCache;

  BlockHashIndex m_hash;
  PhraseDecoder* m_phraseDecoder;

  StringVector<unsigned char, size_t, MmapAllocator>  m_targetPhrasesMapped;
  StringVector<unsigned char, size_t, std::allocator> m_targetPhrasesMemory;

public:
  PhraseDictionaryCompact(const std::string &line);

  ~PhraseDictionaryCompact();

  void Load(AllOptions::ptr const& opts);

  TargetPhraseCollection::shared_ptr  GetTargetPhraseCollectionNonCacheLEGACY(const Phrase &source) const;
  TargetPhraseVectorPtr GetTargetPhraseCollectionRaw(const Phrase &source) const;

  void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

  void CacheForCleanup(TargetPhraseCollection::shared_ptr  tpc);
  void CleanUpAfterSentenceProcessing(const InputType &source);
  static void SetStaticDefaultParameters(Parameter const& param);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &,
    std::size_t) {
    assert(false);
    return 0;
  }

  TO_STRING();

};

}
#endif
