/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "HypoList.h"
#include "Word.h"
#include "WordsRange.h"

namespace search
{
class Vertex;
}

namespace Moses
{

class Word;

/** Contains a range, word (non-terms?) and a vector of hypotheses.
 * @todo This is probably incompatible with lattice decoding when the word that spans
 *   a position (or positions) can vary.
 * @todo is this to hold sorted hypotheses that are in the queue for creating the next hypos?
 */
class ChartCellLabel
{
public:
  union Stack {
    const HypoList *cube; // cube pruning
    search::Vertex *incr; // incremental search after filling.
    void *incr_generator; // incremental search during filling.
  };


  ChartCellLabel(const WordsRange &coverage, const Word &label,
                 Stack stack=Stack())
    : m_coverage(coverage)
    , m_label(label)
    , m_stack(stack) {
  }

  const WordsRange &GetCoverage() const {
    return m_coverage;
  }
  const Word &GetLabel() const {
    return m_label;
  }
  Stack GetStack() const {
    return m_stack;
  }
  Stack &MutableStack() {
    return m_stack;
  }

  bool operator<(const ChartCellLabel &other) const {
    // m_coverage and m_label uniquely identify a ChartCellLabel, so don't
    // need to compare m_stack.
    if (m_coverage == other.m_coverage) {
      return m_label < other.m_label;
    }
    return m_coverage < other.m_coverage;
  }

private:
  const WordsRange &m_coverage;
  const Word &m_label;
  Stack m_stack;
};

}
