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
#pragma once

#include "InputType.h"
#include "ChartCell.h"
#include "WordsRange.h"

#include <boost/ptr_container/ptr_vector.hpp>

namespace Moses
{
class InputType;
class ChartManager;

class ChartCellCollectionBase
{
public:
  template <class Factory> ChartCellCollectionBase(const InputType &input, const Factory &factory) :
    m_cells(input.GetSize()) {
    size_t size = input.GetSize();
    for (size_t startPos = 0; startPos < size; ++startPos) {
      std::vector<ChartCellBase*> &inner = m_cells[startPos];
      inner.reserve(size - startPos);
      for (size_t endPos = startPos; endPos < size; ++endPos) {
        inner.push_back(factory(startPos, endPos));
      }
      /* Hack: ChartCellLabel shouldn't need to know its span, but the parser
       * gets it from there :-(.  The span is actually stored as a reference,
       * which needs to point somewhere, so I have it refer to the ChartCell.
       */
      m_source.push_back(new ChartCellLabel(inner[0]->GetCoverage(), input.GetWord(startPos)));
    }
  }

  virtual ~ChartCellCollectionBase();

  const ChartCellBase &GetBase(const WordsRange &coverage) const {
    return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
  }

  ChartCellBase &MutableBase(const WordsRange &coverage) {
    return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
  }


  const ChartCellLabel &GetSourceWordLabel(size_t at) const {
    return m_source[at];
  }

private:
  std::vector<std::vector<ChartCellBase*> > m_cells;

  boost::ptr_vector<ChartCellLabel> m_source;
};

/** Hold all the chart cells for 1 input sentence. A variable of this type is held by the ChartManager
 */
class ChartCellCollection : public ChartCellCollectionBase
{
public:
  ChartCellCollection(const InputType &input, ChartManager &manager);

  //! get a chart cell for a particular range
  ChartCell &Get(const WordsRange &coverage) {
    return static_cast<ChartCell&>(MutableBase(coverage));
  }

  //! get a chart cell for a particular range
  const ChartCell &Get(const WordsRange &coverage) const {
    return static_cast<const ChartCell&>(GetBase(coverage));
  }
};

}

