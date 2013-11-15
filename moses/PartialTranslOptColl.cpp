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

#include "PartialTranslOptColl.h"
#include <algorithm>
#include <iostream>

using namespace std;

namespace Moses
{
/** constructor, intializes counters and thresholds */
PartialTranslOptColl::PartialTranslOptColl()
{
  m_bestScore = -std::numeric_limits<float>::infinity();
  m_worstScore = -std::numeric_limits<float>::infinity();
  m_maxSize = StaticData::Instance().GetMaxNoPartTransOpt();
  m_totalPruned = 0;
}


/** add a partial translation option to the collection (without pruning) */
void PartialTranslOptColl::AddNoPrune(TranslationOption *partialTranslOpt)
{
  if (partialTranslOpt->GetFutureScore() >= m_worstScore) {
    m_list.push_back(partialTranslOpt);
    if (partialTranslOpt->GetFutureScore() > m_bestScore)
      m_bestScore = partialTranslOpt->GetFutureScore();
  } else {
    m_totalPruned++;
    delete partialTranslOpt;
  }
}

/** add a partial translation option to the collection, prune if necessary.
 * This is done similar to the Prune() in TranslationOptionCollection */

void PartialTranslOptColl::Add(TranslationOption *partialTranslOpt)
{
  // add
  AddNoPrune(partialTranslOpt );

  // done if not too large (lazy pruning, only if twice as large as max)
  if ( m_list.size() > 2 * m_maxSize ) {
    Prune();
  }
}


/** helper, used by pruning */
bool ComparePartialTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
  return a->GetFutureScore() > b->GetFutureScore();
}

/** pruning, remove partial translation options, if list too big */
void PartialTranslOptColl::Prune()
{
  // done if not too big
  if ( m_list.size() <= m_maxSize ) {
    return;
  }

  //	TRACE_ERR( "pruning partial translation options from size " << m_list.size() << std::endl);

  // find nth element
  NTH_ELEMENT4(m_list.begin(),
              m_list.begin() + m_maxSize,
              m_list.end(),
              ComparePartialTranslationOption);

  m_worstScore = m_list[ m_maxSize-1 ]->GetFutureScore();
  // delete the rest
  for (size_t i = m_maxSize ; i < m_list.size() ; ++i) {
    delete m_list[i];
    m_totalPruned++;
  }
  m_list.resize(m_maxSize);
  //	TRACE_ERR( "pruned to size " << m_list.size() << ", total pruned: " << m_totalPruned << std::endl);
}

// friend
ostream& operator<<(ostream& out, const PartialTranslOptColl& possibleTranslation)
{
  for (size_t i = 0; i < possibleTranslation.m_list.size(); ++i) {
    const TranslationOption &transOpt = *possibleTranslation.m_list[i];
    out << transOpt << endl;
  }
  return out;
}

}


