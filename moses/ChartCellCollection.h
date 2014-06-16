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

#include <boost/ptr_container/ptr_vector.hpp>
#include "InputType.h"
#include "ChartCell.h"
#include "WordsRange.h"
#include "InputPath.h"

namespace Moses
{
class InputType;
class ChartManager;

class ChartCellCollectionBase
{
public:
  template <class Factory> ChartCellCollectionBase(const InputType &input, const Factory &factory) :
    m_cells(input.GetSize()) {

	CreateInputPaths(input);

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
      const WordsRange &range = inner[0]->GetCoverage();
      InputPath &path = GetInputPath(range.GetStartPos(), range.GetEndPos());

      m_source.push_back(new ChartCellLabel(range, input.GetWord(startPos)));
    }
  }

  virtual ~ChartCellCollectionBase();

  void CreateInputPaths(const InputType &input)
  {
	  size_t size = input.GetSize();
	  m_inputPathMatrix.resize(size);
	  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
	    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
	      size_t endPos = startPos + phaseSize -1;
	      std::vector<InputPath*> &vec = m_inputPathMatrix[startPos];

	      WordsRange range(startPos, endPos);
	      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
	      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

	      InputPath *path;
	      if (range.GetNumWordsCovered() == 1) {
	        path = new InputPath(subphrase, labels, range, NULL, NULL);
	        vec.push_back(path);
	      } else {
	        const InputPath &prevPath = GetInputPath(startPos, endPos - 1);
	        path = new InputPath(subphrase, labels, range, &prevPath, NULL);
	        vec.push_back(path);
	      }

	      m_inputPathQueue.push_back(path);
	    }
	  }

  }

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

  typedef std::vector< std::vector<InputPath*> > InputPathMatrix;
  InputPathMatrix	m_inputPathMatrix; /*< contains translation options */

  InputPathList m_inputPathQueue;

  InputPath &GetInputPath(size_t startPos, size_t endPos)
  {
    size_t offset = endPos - startPos;
    assert(offset < m_inputPathMatrix[startPos].size());
    return *m_inputPathMatrix[startPos][offset];
  }

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

