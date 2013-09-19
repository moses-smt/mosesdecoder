// $Id: ChartTranslationOptionList.h,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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

#pragma once

#include <queue>
#include <vector>
#include <list>
#include <set>
#include "ChartTranslationOption.h"
#include "ChartTranslationOptionMBOT.h"
#include "TargetPhrase.h"
#include "TargetPhraseMBOT.h"
#include "Util.h"
#include "TargetPhraseCollection.h"
#include "ObjectPool.h"

namespace Moses
{
//! a list of target phrases that is trsnalated from the same source phrase
class ChartTranslationOptionList
{
  friend std::ostream& operator<<(std::ostream&, const ChartTranslationOptionList&);

protected:
#ifdef USE_HYPO_POOL
  static ObjectPool<ChartTranslationOptionList> s_objectPool;
#endif
  typedef std::vector<ChartTranslationOption*> CollType;
  CollType m_collection;
  float m_scoreThreshold;
  WordsRange m_range;

public:
  // iters
  typedef CollType::iterator iterator;
  typedef CollType::const_iterator const_iterator;

  iterator begin() {
    return m_collection.begin();
  }
  iterator end() {
    return m_collection.end();
  }
  const_iterator begin() const {
    return m_collection.begin();
  }
  const_iterator end() const {
    return m_collection.end();
  }

#ifdef USE_HYPO_POOL
  void *operator new(size_t /* num_bytes */) {
    void *ptr = s_objectPool.getPtr();
    return ptr;
  }

  static void Delete(ChartTranslationOptionList *obj) {
    s_objectPool.freeObject(obj);
  }
#else
  static void Delete(ChartTranslationOptionList *obj) {
    delete obj;
  }
#endif

  ChartTranslationOptionList(const WordsRange &range);
  ~ChartTranslationOptionList();

  const ChartTranslationOption &Get(size_t ind) const {
    return *m_collection[ind];
  }

  //FB : get pointer for cast to MBOT tranlsation option
  const ChartTranslationOption * GetPointer(size_t ind) const {
    return m_collection[ind];
  }

  //! divide collection into 2 buckets using std::nth_element, the top & bottom according to table limit
//	void Sort(size_t tableLimit);

  //! number of target phrases in this collection
  size_t GetSize() const {
    return m_collection.size();
  }
  //! wether collection has any phrases
  bool IsEmpty() const {
    return m_collection.empty();
  }

  void Add(const TargetPhraseCollection &targetPhraseCollection
           , const DottedRule &dottedRule
           , const ChartCellCollection &
           , bool ruleLimit
           , size_t tableLimit);

  //new : FB : BEWARE : New method for passing DottedRuleMBOT
  void AddMBOT(const TargetPhraseCollection &targetPhraseCollection
           , const DottedRuleMBOT &dottedRule
           , const ChartCellCollection &
           , bool ruleLimit
           , size_t tableLimit);

  void Add(ChartTranslationOption *transOpt);

  void CreateChartRules(size_t ruleLimit);

  const WordsRange &GetSourceRange() const {
    return m_range;
  }

  void Sort();

  void SortMBOT();


};

}
