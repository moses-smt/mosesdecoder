/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2011 University of Edinburgh

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
#ifndef moses_ChartRuleLookupManager_h
#define moses_ChartRuleLookupManager_h

#include "ChartCellCollection.h"
#include "InputType.h"

namespace Moses
{
class ChartParser;
class ChartParserCallback;
class Range;
class Sentence;

/** Defines an interface for looking up rules in a rule table.  Concrete
 *  implementation classes should correspond to specific PhraseDictionary
 *  subclasses (memory or on-disk).  Since a ChartRuleLookupManager object
 *  maintains sentence-specific state, exactly one should be created for
 *  each sentence that is to be decoded.
 */
class ChartRuleLookupManager
{
public:
  ChartRuleLookupManager(const ChartParser &parser,
                         const ChartCellCollectionBase &cellColl)
    : m_parser(parser)
    , m_cellCollection(cellColl) {}

  virtual ~ChartRuleLookupManager();

  const ChartCellLabelSet &GetTargetLabelSet(size_t begin, size_t end) const {
    return m_cellCollection.GetBase(Range(begin, end)).GetTargetLabelSet();
  }

  const ChartParser &GetParser() const {
    return m_parser;
  }
  //const Sentence &GetSentence() const;

  const ChartCellLabel &GetSourceAt(size_t at) const {
    return m_cellCollection.GetSourceWordLabel(at);
  }

  /** abstract function. Return a vector of translation options for given a range in the input sentence
   *  \param range source range for which you want the translation options
   *  \param outColl return argument
   */
  virtual void GetChartRuleCollection(
    const InputPath &inputPath,
    size_t lastPos,  // last position to consider if using lookahead
    ChartParserCallback &outColl) = 0;

private:
  //! Non-copyable: copy constructor and assignment operator not implemented.
  ChartRuleLookupManager(const ChartRuleLookupManager &);
  //! Non-copyable: copy constructor and assignment operator not implemented.
  ChartRuleLookupManager &operator=(const ChartRuleLookupManager &);

  const ChartParser &m_parser;
  const ChartCellCollectionBase &m_cellCollection;
};

}  // namespace Moses

#endif
