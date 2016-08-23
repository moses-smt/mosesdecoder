// $Id$

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

#ifndef moses_PartialTranslOptColl_h
#define moses_PartialTranslOptColl_h

#include <list>
#include <iostream>
#include "TranslationOption.h"
#include "Util.h"
#include "StaticData.h"
#include "FactorTypeSet.h"

namespace Moses
{

/** Contains partial translation options, while these are constructed in the class TranslationOption.
 *  The factored translation model allows for multiple translation and
 *  generation steps during a single Hypothesis expansion. For efficiency,
 *  all these expansions are precomputed and stored as TranslationOption.
 *  The expansion process itself may be still explode, so efficient handling
 *  of partial translation options during expansion is required.
 *  This class assists in this tasks by implementing pruning.
 *  This implementation is similar to the one in HypothesisStack.
 */
class PartialTranslOptColl
{
  friend std::ostream& operator<<(std::ostream& out, const PartialTranslOptColl& possibleTranslation);

protected:
  typedef std::vector<TranslationOption*> Coll;
  Coll m_list;
  float m_bestScore; /**< score of the best translation option */
  float m_worstScore; /**< score of the worse translation option */
  size_t m_maxSize; /**< maximum number of translation options allowed */
  size_t m_totalPruned; /**< number of options pruned */

public:
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;

  const_iterator begin() const {
    return m_list.begin();
  }
  const_iterator end() const {
    return m_list.end();
  }

  PartialTranslOptColl(size_t const maxSize);

  /** destructor, cleans out list */
  ~PartialTranslOptColl() {
    RemoveAllInColl( m_list );
  }

  void AddNoPrune(TranslationOption *partialTranslOpt);
  void Add(TranslationOption *partialTranslOpt);
  void Prune();

  /** returns list of translation options */
  const std::vector<TranslationOption*>& GetList() const {
    return m_list;
  }

  /** clear out the list */
  void DetachAll() {
    m_list.clear();
  }

  /** return number of pruned partial hypotheses */
  size_t GetPrunedCount() {
    return m_totalPruned;
  }

};

}
#endif
