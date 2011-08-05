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

#include "ChartTrellisPath.h"
#include "ChartHypothesis.h"
#include "ChartTrellisPathCollection.h"
#include "StaticData.h"
#include "Word.h"

using namespace std;

namespace Moses
{

ChartTrellisPath::ChartTrellisPath(const ChartHypothesis *hypo)
  :m_finalNode(new ChartTrellisNode(hypo))
  ,m_scoreBreakdown(hypo->GetScoreBreakdown())
  ,m_totalScore(hypo->GetTotalScore())
  ,m_prevNodeChanged(NULL)
  ,m_prevPath(NULL)
{
}

ChartTrellisPath::ChartTrellisPath(const ChartTrellisPath &origPath
                         , const ChartTrellisNode &soughtNode
                         , const ChartHypothesis &replacementHypo
                         , ScoreComponentCollection	&scoreChange)
  :m_scoreBreakdown(origPath.GetScoreBreakdown())
  ,m_prevPath(&origPath)
{
  m_finalNode = new ChartTrellisNode(origPath.GetFinalNode()
                                , soughtNode
                                , replacementHypo
                                , scoreChange
                                , m_prevNodeChanged);

  m_scoreBreakdown.PlusEquals(scoreChange);

  m_totalScore = m_scoreBreakdown.GetWeightedScore();
}

ChartTrellisPath::~ChartTrellisPath()
{
  delete m_finalNode;
}

Phrase ChartTrellisPath::GetOutputPhrase() const
{
  Phrase ret = GetFinalNode().GetOutputPhrase();
  return ret;
}

void ChartTrellisPath::CreateDeviantPaths(ChartTrellisPathCollection &pathColl, const ChartTrellisNode &soughtNode) const
{
  // copy this path but replace startHypo with its arc
  const ChartArcList *arcList = soughtNode.GetHypothesis().GetArcList();

  if (arcList) {
    ChartArcList::const_iterator iterChartArcList;
    for (iterChartArcList = arcList->begin(); iterChartArcList != arcList->end(); ++iterChartArcList) {
      ScoreComponentCollection	scoreChange;

      const ChartHypothesis &replacementHypo = **iterChartArcList;
      ChartTrellisPath *newPath = new ChartTrellisPath(*this, soughtNode, replacementHypo, scoreChange);
      pathColl.Add(newPath);
    }
  }

  // recusively create deviant paths for child nodes
  const ChartTrellisNode::NodeChildren &children = soughtNode.GetChildren();

  ChartTrellisNode::NodeChildren::const_iterator iter;
  for (iter = children.begin(); iter != children.end(); ++iter) {
    const ChartTrellisNode &child = **iter;
    CreateDeviantPaths(pathColl, child);
  }
}

void ChartTrellisPath::CreateDeviantPaths(ChartTrellisPathCollection &pathColl) const
{
  if (m_prevNodeChanged == NULL) {
    // initial enumeration from a pure hypo
    CreateDeviantPaths(pathColl, GetFinalNode());
  } else {
    // don't change m_prevNodeChanged, just it's children
    const ChartTrellisNode::NodeChildren &children = m_prevNodeChanged->GetChildren();

    ChartTrellisNode::NodeChildren::const_iterator iter;
    for (iter = children.begin(); iter != children.end(); ++iter) {
      const ChartTrellisNode &child = **iter;
      CreateDeviantPaths(pathColl, child);
    }
  }
}

std::ostream& operator<<(std::ostream &out, const ChartTrellisPath &path)
{
  out << &path << "  " << path.m_prevPath << "  " << path.GetOutputPhrase() << endl;

  if (path.m_prevNodeChanged) {
    out << "changed " << path.m_prevNodeChanged->GetHypothesis() << endl;
  }

  out << path.GetFinalNode() << endl;

  return out;
}

};

