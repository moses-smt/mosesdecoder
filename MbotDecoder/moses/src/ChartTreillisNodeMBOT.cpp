// $Id: ChartTreillisNodeMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
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

#include "ChartTreillisNodeMBOT.h"

#include "ChartHypothesisMBOT.h"
#include "ChartTreillisDetourMBOT.h"
#include "ChartTreillisPathMBOT.h"
#include "StaticData.h"
#include "DotChart.h"

namespace Moses
{

ChartTreillisNodeMBOT::ChartTreillisNodeMBOT(const ChartHypothesisMBOT &hypo)
    : m_mbotHypo(hypo)
{
  CreateChildrenMBOT();
}

ChartTreillisNodeMBOT::ChartTreillisNodeMBOT(const ChartTreillisDetourMBOT &detour,
                                   ChartTreillisNodeMBOT *&deviationPoint)
    : m_mbotHypo((&detour.GetBasePathMBOT().GetFinalNodeMBOT() == &detour.GetSubstitutedNodeMBOT())
             ? detour.GetReplacementHypoMBOT()
             : detour.GetBasePathMBOT().GetFinalNodeMBOT().GetHypothesisMBOT())
{
  if (&m_mbotHypo == &detour.GetReplacementHypoMBOT()) {
    deviationPoint = this;
    CreateChildrenMBOT();
  } else {
    CreateChildrenMBOT(detour.GetBasePathMBOT().GetFinalNodeMBOT(),
                   detour.GetSubstitutedNodeMBOT(), detour.GetReplacementHypoMBOT(),
                   deviationPoint);
  }
}

ChartTreillisNodeMBOT::ChartTreillisNodeMBOT(const ChartTreillisNodeMBOT &root,
                                   const ChartTreillisNodeMBOT &substitutedNode,
                                   const ChartHypothesisMBOT &replacementHypo,
                                   ChartTreillisNodeMBOT *&deviationPoint)
    : m_mbotHypo((&root == &substitutedNode)
             ? replacementHypo
             : root.GetHypothesisMBOT())
{
  if (&root == &substitutedNode) {
    deviationPoint = this;
    CreateChildrenMBOT();
  } else {
    CreateChildrenMBOT(root, substitutedNode, replacementHypo, deviationPoint);
  }
}

ChartTreillisNodeMBOT::~ChartTreillisNodeMBOT()
{
  RemoveAllInColl(m_mbotChildren);
}

Phrase ChartTreillisNodeMBOT::GetOutputPhrase(ProcessedNonTerminals * processedNonTerminals) const
{
  // exactly like same fn in hypothesis, but use trellis nodes instead of prevHypos pointer
  Phrase ret(ARRAY_SIZE_INCR);


   // add word as parameter
   processedNonTerminals->AddHypothesis(&m_mbotHypo);
   processedNonTerminals->AddStatus(m_mbotHypo.GetId(),1);

   const ChartHypothesisMBOT * currentHypo = processedNonTerminals->GetHypothesis();

   //look at size : if there is only one m_mbot phrase, process as usual
   //if there are several mbot phrases : split hypotheses

   std::vector<Phrase> targetPhrases = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases();
   const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTAlignments();

   int currentlyProcessed = processedNonTerminals->GetStatus(currentHypo->GetId()) -1;

   CHECK(currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() > currentlyProcessed);
   size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();

   //if several mbot phrases, look at status
   Phrase currentPhrase = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed];

   int position = 0;
   	   for (size_t pos = 0; pos < currentPhrase.GetSize(); ++pos) {
   		   const Word &word = currentPhrase.GetWord(pos);
           if (word.IsNonTerminal()) {
        	   const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
               alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();
        	   int sizeOfMap = nonTermIndexMap->size();
               // non-term. fill out with prev hypo
               //get hypo corresponding to mbot phrase
               size_t nonTermInd = nonTermIndexMap->at(pos);
               processedNonTerminals->IncrementRec();
               const ChartTreillisNodeMBOT &childNode = GetChildMBOT(nonTermInd);
               Phrase childPhrase = childNode.GetOutputPhrase(processedNonTerminals);
               ret.Append(childPhrase);}
               else {
                ret.AddWord(word);
                //Add processed leaves to processed span unless leaf is mbot target phrase
                }
                if(mbotSize > (processedNonTerminals->GetStatus(currentHypo->GetId())) && (pos == currentPhrase.GetSize() - 1) )
                {
                	processedNonTerminals->IncrementStatus(currentHypo->GetId());
                }
        }
        //mark this hypothesis as done.
        processedNonTerminals->DecrementRec();
        return ret;
}

void ChartTreillisNodeMBOT::CreateChildrenMBOT()
{
  CHECK(m_mbotChildren.empty());
  const std::vector<const ChartHypothesisMBOT*> &prevHypos = m_mbotHypo.GetPrevHyposMBOT();
  m_mbotChildren.reserve(prevHypos.size());
  for (size_t ind = 0; ind < prevHypos.size(); ++ind) {
    const ChartHypothesisMBOT *prevHypo = prevHypos[ind];
    ChartTreillisNodeMBOT *child = new ChartTreillisNodeMBOT(*prevHypo);
    m_mbotChildren.push_back(child);
  }
}

void ChartTreillisNodeMBOT::CreateChildrenMBOT(const ChartTreillisNodeMBOT &rootNode,
                                      const ChartTreillisNodeMBOT &substitutedNode,
                                      const ChartHypothesisMBOT &replacementHypo,
                                      ChartTreillisNodeMBOT *&deviationPoint)
{
  CHECK(m_mbotChildren.empty());
  const NodeChildrenMBOT &children = rootNode.GetChildrenMBOT();
  m_mbotChildren.reserve(children.size());
  for (size_t ind = 0; ind < children.size(); ++ind) {
    const ChartTreillisNodeMBOT *origChild = children[ind];
    ChartTreillisNodeMBOT *child = new ChartTreillisNodeMBOT(*origChild, substitutedNode,
                                                   replacementHypo,
                                                   deviationPoint);
    m_mbotChildren.push_back(child);
  }
}

}
