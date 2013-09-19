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

  //const ChartTranslationOption &transOpt = m_mbotHypo.GetTranslationOption();

  //VERBOSE(3, "Trans Opt:" << transOpt.GetDottedRule() << ": " << m_hypo.GetCurrTargetPhrase().GetTargetLHS() << "->" << m_hypo.GetCurrTargetPhrase() << std::endl);


   // add word as parameter
   //std::cout << "AT BEGINNING OF METHOD : ADDED HYPOTHESIS : " << m_mbotHypo << " : " << m_mbotHypo.GetCurrTargetPhraseMBOT() << std::endl;
   processedNonTerminals->AddHypothesis(&m_mbotHypo);
   processedNonTerminals->AddStatus(m_mbotHypo.GetId(),1);

   //std::cout << "NUMBER OF RECURSION " << processedNonTerminals->GetRecNumber() << std::endl;
   const ChartHypothesisMBOT * currentHypo = processedNonTerminals->GetHypothesis();
   //std::cout << "CURRENT HYPOTHESIS : " << currentHypo << std::endl;
   //const TargetPhraseMBOT currentTarget = currentHypo->GetCurrTargetPhraseMBOT();
   //std::cout << "CREATING TARGET PHRASE : " << currentTarget << std::endl;

   //look at size : if there is only one m_mbot phrase, process as usual
   //if there are several mbot phrases : split hypotheses

   std::vector<Phrase> targetPhrases = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases();
   const std::vector<const AlignmentInfoMBOT*> *alignedTargets = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTAlignments();
   //std::vector<Word> targetLHS = currentHypo->GetCurrTargetPhraseMBOT().GetTargetLHSMBOT();

   //while(GetProcessingPhrase()->GetStatus() < targetPhrases.size())
   //{
        int currentlyProcessed = processedNonTerminals->GetStatus(currentHypo->GetId()) -1;
       //std::cout << "CURRENTLY PROCESSED : " << currentlyProcessed << std::endl;

        //GetProcessingPhrase()->IncrementStatus();
        //std::cout << "CURENT STATUS : " << GetProcessingPhrase()->GetStatus() << std::endl;

        CHECK(currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() > currentlyProcessed);
        size_t mbotSize = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size();
        //std::cout << "Getting current Phrase : " << GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() << std::endl;

        //if several mbot phrases, look at status
         Phrase currentPhrase = currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases()[currentlyProcessed];
         //std::cout << "Current MBOT Phrase : " << currentPhrase << std::endl;
        //std::vector<Phrase>::iterator itr_mbot_phrases;

        //if(targetPhrases.size() == 1)
        //{

         //if several mbot phrases, look at status
        //if(mbotSize > 1)
        //{
           int position = 0;
        //Phrase currentPhrase = targetPhrases.front();
            //std::cout << "CURRENT PHRASE : " << currentPhrase << std::endl;
            for (size_t pos = 0; pos < currentPhrase.GetSize(); ++pos) {

                //std::cout << "POSITION : " << pos << std::endl;
                const Word &word = currentPhrase.GetWord(pos);
                //std::cout << "CURRENT WORD : " << word << std::endl;
                if (word.IsNonTerminal()) {
                    const AlignmentInfoMBOT::NonTermIndexMapPointer nonTermIndexMap =
                    alignedTargets->at(currentlyProcessed)->GetNonTermIndexMap();

                    int sizeOfMap = nonTermIndexMap->size();
                    //std::cout << "MBOT current map : " << sizeOfMap << std::endl;
                // non-term. fill out with prev hypo
                //get hypo corresponding to mbot phrase
                size_t nonTermInd = nonTermIndexMap->at(pos);
                //std::cout << "NON TERM IND : " << nonTermInd << std::endl;
                processedNonTerminals->IncrementRec();
                //std::cout << "TAKING PREVIOUS HYPO of " << *currentHypo << std::endl;
                const ChartTreillisNodeMBOT &childNode = GetChildMBOT(nonTermInd);
                //MBOT CONDITION HERE ::::
                Phrase childPhrase = childNode.GetOutputPhrase(processedNonTerminals);
                ret.Append(childPhrase);
                }
                else {
                //std::cout << "ADDED WORD :" << word << std::endl;
                ret.AddWord(word);
                //std::cout << "MBOT SIZE : " << mbotSize << " : Status : " << processedNonTerminals->GetStatus(currentHypo->GetId()) << std::endl;
                //std::cout << "CURRENT POSITION : " << pos << std::endl;
                //Add processed leaves to processed span unless leaf is mbot target phrase
                }
                //std::cout << "MbotSize and status " << mbotSize << " : " << processedNonTerminals.GetStatus(currentHypo->GetId()) << std::endl;
                 if(//(currentHypo->GetCurrSourceRange().GetStartPos() == currentHypo->GetCurrSourceRange().GetEndPos())
                    //&& (
                        mbotSize > (processedNonTerminals->GetStatus(currentHypo->GetId()))
                          //)
                        && (pos == currentPhrase.GetSize() - 1) )
                        {
                            processedNonTerminals->IncrementStatus(currentHypo->GetId());
                            }
                /*if(
                   (pos == currentPhrase.GetSize() - 1)
                   //(currentHypo->GetCurrSourceRange().GetStartPos() == currentHypo->GetCurrSourceRange().GetEndPos())
                   && ( mbotSize == (currentHypo->GetProcessingPhrase()->GetStatus() ) || mbotSize == 1))
                   //&& (currentHypo->GetCurrTargetPhraseMBOT().GetMBOTPhrases().size() == 1))
                   processedNonTerminals.AddRangeMergeAll(currentHypo->GetCurrSourceRange());
                }*/
        }
        //std::cout << "GOES OUT OF RECURSION" << std::endl;
        //std::cout << "CURRENT HYPOTHESIS : " << (*currentHypo) << std::endl;
        //mark this hypothesis as done.
        processedNonTerminals->DecrementRec();
        /*if(processedNonTerminals->GetRecNumber() < -1)
        {
            processedNonTerminals->IncrementRec();
        }*/
        return ret;
    //}
//std::cout << "THIS HYPOTHESIS : " << *prevHypo << std::endl;
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
