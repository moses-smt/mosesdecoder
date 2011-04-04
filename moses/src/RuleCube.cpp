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

#include "RuleCube.h"
#include "ChartCell.h"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "RuleCubeQueue.h"
#include "WordsRange.h"
#include "ChartTranslationOption.h"
#include "Util.h"
#include "CoveredChartSpan.h"

#ifdef HAVE_BOOST
#include <boost/functional/hash.hpp>
#endif

using namespace std;
using namespace Moses;

namespace Moses
{

// create a cube for a rule
RuleCube::RuleCube(const ChartTranslationOption &transOpt
                       , const ChartCellCollection &allChartCells)
  :m_transOpt(transOpt)
{
  const CoveredChartSpan *coveredChartSpan = &transOpt.GetLastCoveredChartSpan();
  CreateRuleCubeDimension(coveredChartSpan, allChartCells);
  CalcScore();
}

// for each non-terminal, create a ordered list of matching hypothesis from the chart
void RuleCube::CreateRuleCubeDimension(const CoveredChartSpan *coveredChartSpan, const ChartCellCollection &allChartCells)
{
  // recurse through the linked list of source side non-terminals and terminals
  const CoveredChartSpan *prevCoveredChartSpan = coveredChartSpan->GetPrevCoveredChartSpan();
  if (prevCoveredChartSpan)
    CreateRuleCubeDimension(prevCoveredChartSpan, allChartCells);

  // only deal with non-terminals
  if (coveredChartSpan->IsNonTerminal()) 
  {
    // get the essential information about the non-terminal
    const WordsRange &childRange = coveredChartSpan->GetWordsRange(); // span covered by child
    const ChartCell &childCell = allChartCells.Get(childRange);    // list of all hypos for that span
    const Word &nonTerm = coveredChartSpan->GetSourceWord();          // target (sic!) non-terminal label 

    // there have to be hypothesis with the desired non-terminal
    // (otherwise the rule would not be considered)
    assert(!childCell.GetSortedHypotheses(nonTerm).empty());

    // create a list of hypotheses that match the non-terminal
    RuleCubeDimension ruleCubeDimension(0, childCell.GetSortedHypotheses(nonTerm));
    // add them to the vector for such lists
    m_cube.push_back(ruleCubeDimension);
  }
}

// create the RuleCube from an existing one, differing only in one child hypothesis
RuleCube::RuleCube(const RuleCube &copy, size_t ruleCubeDimensionIncr)
  :m_transOpt(copy.m_transOpt)
  ,m_cube(copy.m_cube)
{
  RuleCubeDimension &ruleCubeDimension = m_cube[ruleCubeDimensionIncr];
  ruleCubeDimension.IncrementPos();
  CalcScore();
}

RuleCube::~RuleCube()
{
  //RemoveAllInColl(m_cube);
}

// create new RuleCube for neighboring principle rules
// (duplicate detection is handled in RuleCubeQueue)
void RuleCube::CreateNeighbors(RuleCubeQueue &queue) const
{
  // loop over all child hypotheses
  for (size_t ind = 0; ind < m_cube.size(); ind++) {
    const RuleCubeDimension &ruleCubeDimension = m_cube[ind];

    if (ruleCubeDimension.HasMoreHypo()) {
      RuleCube *newEntry = new RuleCube(*this, ind);
      queue.Add(newEntry);
    }
  }
}

// compute an estimated cost of the principle rule
// (consisting of rule translation scores plus child hypotheses scores)
void RuleCube::CalcScore()
{
  m_combinedScore = m_transOpt.GetTargetPhrase().GetFutureScore();
  for (size_t ind = 0; ind < m_cube.size(); ind++) {
    const RuleCubeDimension &ruleCubeDimension = m_cube[ind];

    const ChartHypothesis *hypo = ruleCubeDimension.GetHypothesis();
    m_combinedScore += hypo->GetTotalScore();
  }
}

bool RuleCube::operator<(const RuleCube &compare) const
{
  if (&m_transOpt != &compare.m_transOpt)
    return &m_transOpt < &compare.m_transOpt;

  bool ret = m_cube < compare.m_cube;
  return ret;
}

#ifdef HAVE_BOOST
std::size_t hash_value(const RuleCubeDimension & ruleCubeDimension)
{
  boost::hash<const ChartHypothesis*> hasher;
  return hasher(ruleCubeDimension.GetHypothesis());
}

#endif
std::ostream& operator<<(std::ostream &out, const RuleCubeDimension &ruleCubeDimension)
{
  out << *ruleCubeDimension.GetHypothesis();
  return out;
}

std::ostream& operator<<(std::ostream &out, const RuleCube &ruleCube)
{
  out << ruleCube.GetTranslationOption() << endl;
  std::vector<RuleCubeDimension>::const_iterator iter;
  for (iter = ruleCube.GetCube().begin(); iter != ruleCube.GetCube().end(); ++iter) {
    out << *iter << endl;
  }
  return out;
}

}

