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

#include <algorithm>
#include "TargetPhraseCollection.h"

using namespace std;

namespace Moses
{
// helper for sort
struct CompareTargetPhrase {
  bool operator() (const TargetPhrase *a, const TargetPhrase *b) {
    return a->GetFutureScore() > b->GetFutureScore();
  }
};

void TargetPhraseCollection::NthElement(size_t tableLimit)
{
  vector<TargetPhrase*>::iterator
  iterMiddle = (tableLimit == 0 || m_collection.size() < tableLimit) ?m_collection.end() : m_collection.begin() + tableLimit;

  //std::sort(m_collection.begin(), m_collection.end(), CompareTargetPhrase());
  std::nth_element(m_collection.begin(), iterMiddle, m_collection.end(), CompareTargetPhrase());
}

void TargetPhraseCollection::Prune(bool adhereTableLimit, size_t tableLimit)
{
  NthElement(tableLimit);

  if (adhereTableLimit && m_collection.size() > tableLimit) {
    for (size_t ind = tableLimit; ind < m_collection.size(); ++ind) {
      TargetPhrase *targetPhrase = m_collection[ind];
      delete targetPhrase;
    }
    m_collection.erase(m_collection.begin() + tableLimit, m_collection.end());
  }
}

void TargetPhraseCollection::Sort(bool adhereTableLimit, size_t tableLimit)
{
  std::vector<TargetPhrase*>::iterator iterMiddle;
  iterMiddle = (tableLimit == 0 || m_collection.size() < tableLimit)
             ? m_collection.end()
             : m_collection.begin()+tableLimit;

  std::partial_sort(m_collection.begin(), iterMiddle, m_collection.end(),
                    CompareTargetPhrase());

  if (adhereTableLimit && m_collection.size() > tableLimit) {
    for (size_t i = tableLimit; i < m_collection.size(); ++i) {
      TargetPhrase *targetPhrase = m_collection[i];
      delete targetPhrase;
    }
    m_collection.erase(m_collection.begin()+tableLimit, m_collection.end());
  }
}

}


