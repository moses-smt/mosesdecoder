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

#include "ChartTrellisNode.h"
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"

using namespace std;

namespace Moses
{

ChartTrellisNode::ChartTrellisNode(const ChartHypothesis *hypo)
  :m_hypo(hypo)
{
  const std::vector<const ChartHypothesis*> &prevHypos = hypo->GetPrevHypos();

  m_edge.reserve(prevHypos.size());
  for (size_t ind = 0; ind < prevHypos.size(); ++ind) {
    const ChartHypothesis *prevHypo = prevHypos[ind];
    ChartTrellisNode *child = new ChartTrellisNode(prevHypo);
    m_edge.push_back(child);
  }

  assert(m_hypo);
}

ChartTrellisNode::ChartTrellisNode(const ChartTrellisNode &origNode
                         , const ChartTrellisNode &soughtNode
                         , const ChartHypothesis &replacementHypo
                         , ScoreComponentCollection	&scoreChange
                         , const ChartTrellisNode *&nodeChanged)
{
  if (&origNode.GetHypothesis() == &soughtNode.GetHypothesis()) {
    // this node should be replaced
    m_hypo = &replacementHypo;
    nodeChanged = this;

    // scores
    assert(scoreChange.GetWeightedScore() == 0); // should only be changing 1 node

    scoreChange = replacementHypo.GetScoreBreakdown();
    scoreChange.MinusEquals(origNode.GetHypothesis().GetScoreBreakdown());

    float deltaScore = scoreChange.GetWeightedScore();
    assert(deltaScore <= 0.0005);

    // follow prev hypos back to beginning
    const std::vector<const ChartHypothesis*> &prevHypos = replacementHypo.GetPrevHypos();
    vector<const ChartHypothesis*>::const_iterator iter;
    assert(m_edge.empty());
    m_edge.reserve(prevHypos.size());
    for (iter = prevHypos.begin(); iter != prevHypos.end(); ++iter) {
      const ChartHypothesis *prevHypo = *iter;
      ChartTrellisNode *prevNode = new ChartTrellisNode(prevHypo);
      m_edge.push_back(prevNode);
    }

  } else {
    // not the node we're looking for. Copy as-is and continue finding node
    m_hypo = &origNode.GetHypothesis();
    NodeChildren::const_iterator iter;
    assert(m_edge.empty());
    m_edge.reserve(origNode.m_edge.size());
    for (iter = origNode.m_edge.begin(); iter != origNode.m_edge.end(); ++iter) {
      const ChartTrellisNode &prevNode = **iter;
      ChartTrellisNode *newPrevNode = new ChartTrellisNode(prevNode, soughtNode, replacementHypo, scoreChange, nodeChanged);
      m_edge.push_back(newPrevNode);
    }
  }

  assert(m_hypo);
}

ChartTrellisNode::~ChartTrellisNode()
{
  RemoveAllInColl(m_edge);
}

Phrase ChartTrellisNode::GetOutputPhrase() const
{
  // exactly like same fn in hypothesis, but use trellis nodes instead of prevHypos pointer
  Phrase ret(Output, ARRAY_SIZE_INCR);

  const Phrase &currTargetPhrase = m_hypo->GetCurrTargetPhrase();
  for (size_t pos = 0; pos < currTargetPhrase.GetSize(); ++pos) {
    const Word &word = currTargetPhrase.GetWord(pos);
    if (word.IsNonTerminal()) {
      // non-term. fill out with prev hypo
      size_t nonTermInd = m_hypo->GetCoveredChartSpanTargetOrder(pos);
      const ChartTrellisNode &childNode = GetChild(nonTermInd);
      Phrase childPhrase = childNode.GetOutputPhrase();
      ret.Append(childPhrase);
    } else {
      ret.AddWord(word);
    }
  }

  return ret;
}

std::ostream& operator<<(std::ostream &out, const ChartTrellisNode &node)
{
  out << "*   " << node.GetHypothesis() << endl;

  ChartTrellisNode::NodeChildren::const_iterator iter;
  for (iter = node.GetChildren().begin(); iter != node.GetChildren().end(); ++iter) {
    out << **iter;
  }

  return out;
}

}

