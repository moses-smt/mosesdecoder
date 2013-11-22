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
#include "ChartTrellisDetour.h"
#include "ChartTrellisPath.h"
#include "StaticData.h"
#include "util/exception.hh"

namespace Moses
{

ChartTrellisNode::ChartTrellisNode(const ChartHypothesis &hypo)
  : m_hypo(hypo)
{
  CreateChildren();
}

ChartTrellisNode::ChartTrellisNode(const ChartTrellisDetour &detour,
                                   ChartTrellisNode *&deviationPoint)
  : m_hypo((&detour.GetBasePath().GetFinalNode() == &detour.GetSubstitutedNode())
           ? detour.GetReplacementHypo()
           : detour.GetBasePath().GetFinalNode().GetHypothesis())
{
  if (&m_hypo == &detour.GetReplacementHypo()) {
    deviationPoint = this;
    CreateChildren();
  } else {
    CreateChildren(detour.GetBasePath().GetFinalNode(),
                   detour.GetSubstitutedNode(), detour.GetReplacementHypo(),
                   deviationPoint);
  }
}

ChartTrellisNode::ChartTrellisNode(const ChartTrellisNode &root,
                                   const ChartTrellisNode &substitutedNode,
                                   const ChartHypothesis &replacementHypo,
                                   ChartTrellisNode *&deviationPoint)
  : m_hypo((&root == &substitutedNode)
           ? replacementHypo
           : root.GetHypothesis())
{
  if (&root == &substitutedNode) {
    deviationPoint = this;
    CreateChildren();
  } else {
    CreateChildren(root, substitutedNode, replacementHypo, deviationPoint);
  }
}

ChartTrellisNode::~ChartTrellisNode()
{
  RemoveAllInColl(m_children);
}

Phrase ChartTrellisNode::GetOutputPhrase() const
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

  // exactly like same fn in hypothesis, but use trellis nodes instead of prevHypos pointer
  Phrase ret(ARRAY_SIZE_INCR);

  const Phrase &currTargetPhrase = m_hypo.GetCurrTargetPhrase();
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    m_hypo.GetCurrTargetPhrase().GetAlignNonTerm().GetNonTermIndexMap();
  for (size_t pos = 0; pos < currTargetPhrase.GetSize(); ++pos) {
    const Word &word = currTargetPhrase.GetWord(pos);
    if (word.IsNonTerminal()) {
      // non-term. fill out with prev hypo
      size_t nonTermInd = nonTermIndexMap[pos];
      const ChartTrellisNode &childNode = GetChild(nonTermInd);
      Phrase childPhrase = childNode.GetOutputPhrase();
      ret.Append(childPhrase);
    } else {
      ret.AddWord(word);

      if (placeholderFactor != NOT_FOUND) {
        std::set<size_t> sourcePosSet = m_hypo.GetCurrTargetPhrase().GetAlignTerm().GetAlignmentsForTarget(pos);
        if (sourcePosSet.size() == 1) {
          const std::vector<const Word*> *ruleSourceFromInputPath = m_hypo.GetTranslationOption().GetSourceRuleFromInputPath();
          UTIL_THROW_IF2(ruleSourceFromInputPath == NULL, "Source Words in of the rules hasn't been filled out");

          size_t sourcePos = *sourcePosSet.begin();
          const Word *sourceWord = ruleSourceFromInputPath->at(sourcePos);
          UTIL_THROW_IF2(sourceWord == NULL, "Null source word at position " << sourcePos);
          const Factor *factor = sourceWord->GetFactor(placeholderFactor);
          if (factor) {
            ret.Back()[0] = factor;
          }
        }
      }
    }
  }

  return ret;
}

void ChartTrellisNode::CreateChildren()
{
  UTIL_THROW_IF2(!m_children.empty(), "Already created children");

  const std::vector<const ChartHypothesis*> &prevHypos = m_hypo.GetPrevHypos();
  m_children.reserve(prevHypos.size());
  for (size_t ind = 0; ind < prevHypos.size(); ++ind) {
    const ChartHypothesis *prevHypo = prevHypos[ind];
    ChartTrellisNode *child = new ChartTrellisNode(*prevHypo);
    m_children.push_back(child);
  }
}

void ChartTrellisNode::CreateChildren(const ChartTrellisNode &rootNode,
                                      const ChartTrellisNode &substitutedNode,
                                      const ChartHypothesis &replacementHypo,
                                      ChartTrellisNode *&deviationPoint)
{
  UTIL_THROW_IF2(!m_children.empty(), "Already created children");

  const NodeChildren &children = rootNode.GetChildren();
  m_children.reserve(children.size());
  for (size_t ind = 0; ind < children.size(); ++ind) {
    const ChartTrellisNode *origChild = children[ind];
    ChartTrellisNode *child = new ChartTrellisNode(*origChild, substitutedNode,
        replacementHypo,
        deviationPoint);
    m_children.push_back(child);
  }
}

}
