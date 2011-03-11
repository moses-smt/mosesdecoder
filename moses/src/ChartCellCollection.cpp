// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "ChartCellCollection.h"
#include "InputType.h"
#include "WordsRange.h"

namespace Moses
{
ChartCellCollection::ChartCellCollection(const InputType &input, ChartManager &manager)
  :m_hypoStackColl(input.GetSize())
{
  size_t size = input.GetSize();
  for (size_t startPos = 0; startPos < size; ++startPos) {
    InnerCollType &inner = m_hypoStackColl[startPos];
    inner.resize(size - startPos);

    size_t ind = 0;
    for (size_t endPos = startPos ; endPos < size; ++endPos) {
      ChartCell *cell = new ChartCell(startPos, endPos, manager);
      inner[ind] = cell;
      ++ind;
    }
  }
}

ChartCellCollection::~ChartCellCollection()
{
  OuterCollType::iterator iter;
  for (iter = m_hypoStackColl.begin(); iter != m_hypoStackColl.end(); ++iter) {
    InnerCollType &inner = *iter;
    RemoveAllInColl(inner);
  }
}

} // namespace

