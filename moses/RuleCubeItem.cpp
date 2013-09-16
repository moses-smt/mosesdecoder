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

#include "ChartCell.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptions.h"
#include "RuleCubeItem.h"
#include "RuleCubeQueue.h"
#include "WordsRange.h"
#include "Util.h"
#include "TargetPhraseMBOT.h"

#include <boost/functional/hash.hpp>

namespace Moses
{

const TargetPhraseMBOT *TranslationDimension::GetTargetPhraseMBOT() const {
    	const TargetPhrase * tpConst = (*m_orderedTargetPhrases)[m_pos];
    	CHECK(tpConst);
    	std::cerr << "tpConst=" << *tpConst << std::endl;

    	const TargetPhraseMBOT * tpmbot = dynamic_cast<const TargetPhraseMBOT*>(tpConst);
    	CHECK(tpmbot);

    	//const PhraseSequence *sequence = tpmbot->GetMBOTPhrases();
    	//assert(sequence);

    	return tpmbot;
}

size_t TranslationDimension::GetPositionOfMatchingTargetPhrase() const {
    size_t index_to_check = m_pos;
    	    while(index_to_check < m_orderedTargetPhrases->size())
    	    {
    	    		if(static_cast<TargetPhraseMBOT*>((*m_orderedTargetPhrases)[index_to_check])->isMatchesSource())
    	    		{
    	    			return index_to_check;
    	    		}
    	    		 index_to_check++;
    	    }
    	    return 0;
     }

bool TranslationDimension::HasMoreMatchingTargetPhrase() const {
	size_t index_to_check = m_pos;
	while(index_to_check < m_orderedTargetPhrases->size())
	{
	    if(static_cast<TargetPhraseMBOT*>((*m_orderedTargetPhrases)[index_to_check])->isMatchesSource())
	    {
	    	return true;
	    }
	    index_to_check++;
	}
	return false;
}

std::size_t hash_value(const HypothesisDimension &dimension)
{
  boost::hash<const ChartHypothesis*> hasher;
  return hasher(dimension.GetHypothesis());
}

RuleCubeItem::RuleCubeItem(const ChartTranslationOptions &transOpt,
                           const ChartCellCollection &/*allChartCells*/)
  : m_translationDimension(0,
                           transOpt.GetTargetPhraseCollection().GetCollection())
  , m_hypothesis(0)
{
	const vector<TargetPhrase*> &coll = transOpt.GetTargetPhraseCollection().GetCollection();
	for (TargetPhraseCollection::const_iterator iter = coll.begin(); iter !=  coll.end(); iter++)
	{
		 const TargetPhraseMBOT *tpmbot = dynamic_cast<TargetPhraseMBOT*>(*iter);
		 CHECK(tpmbot);
		cerr << "TPC" << *tpmbot << endl;
	}


  CreateHypothesisDimensions(transOpt.GetStackVec());
}

// create the RuleCube from an existing one, differing only in one dimension
RuleCubeItem::RuleCubeItem(const RuleCubeItem &copy, int hypoDimensionIncr)
  : m_translationDimension(copy.m_translationDimension)
  , m_hypothesisDimensions(copy.m_hypothesisDimensions)
  , m_hypothesis(0)
{
  if (hypoDimensionIncr == -1) {
    m_translationDimension.IncrementPos();
  } else {
    HypothesisDimension &dimension = m_hypothesisDimensions[hypoDimensionIncr];
    dimension.IncrementPos();
  }
}

RuleCubeItem::~RuleCubeItem()
{
  delete m_hypothesis;
}

void RuleCubeItem::EstimateScore()
{
  m_score = m_translationDimension.GetTargetPhrase()->GetFutureScore();
  std::vector<HypothesisDimension>::const_iterator p;
  for (p = m_hypothesisDimensions.begin();
       p != m_hypothesisDimensions.end(); ++p) {
    m_score += p->GetHypothesis()->GetTotalScore();
  }
}

void RuleCubeItem::CreateHypothesis(const ChartTranslationOptions &transOpt,
                                    ChartManager &manager)
{
  m_hypothesis = new ChartHypothesis(transOpt, *this, manager);
  m_hypothesis->CalcScore();
  m_score = m_hypothesis->GetTotalScore();
}

ChartHypothesis *RuleCubeItem::ReleaseHypothesis()
{
  CHECK(m_hypothesis);
  ChartHypothesis *hypo = m_hypothesis;
  m_hypothesis = 0;
  return hypo;
}

// for each non-terminal, create a ordered list of matching hypothesis from the
// chart
void RuleCubeItem::CreateHypothesisDimensions(const StackVec &stackVec)
{
  for (StackVec::const_iterator p = stackVec.begin(); p != stackVec.end();
       ++p) {
    const HypoList *stack = (*p)->GetStack().cube;
    assert(stack);

    // there have to be hypothesis with the desired non-terminal
    // (otherwise the rule would not be considered)
    assert(!stack->empty());

    // create a list of hypotheses that match the non-terminal
    HypothesisDimension dimension(0, *stack);
    // add them to the vector for such lists
    m_hypothesisDimensions.push_back(dimension);
  }
}

bool RuleCubeItem::operator<(const RuleCubeItem &compare) const
{
  if (m_translationDimension == compare.m_translationDimension) {
    return m_hypothesisDimensions < compare.m_hypothesisDimensions;
  }
  return m_translationDimension < compare.m_translationDimension;
}

}
