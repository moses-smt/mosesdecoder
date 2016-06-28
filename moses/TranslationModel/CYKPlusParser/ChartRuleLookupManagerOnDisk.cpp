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

#include "ChartRuleLookupManagerOnDisk.h"

#include <algorithm>

#include "moses/ChartParser.h"
#include "moses/TranslationModel/RuleTable/PhraseDictionaryOnDisk.h"
#include "moses/StaticData.h"
#include "moses/ChartParserCallback.h"
#include "DotChartOnDisk.h"
#include "OnDiskPt/TargetPhraseCollection.h"

using namespace std;

namespace Moses
{

ChartRuleLookupManagerOnDisk::ChartRuleLookupManagerOnDisk(
  const ChartParser &parser,
  const ChartCellCollectionBase &cellColl,
  const PhraseDictionaryOnDisk &dictionary,
  OnDiskPt::OnDiskWrapper &dbWrapper,
  const std::vector<FactorType> &inputFactorsVec,
  const std::vector<FactorType> &outputFactorsVec)
  : ChartRuleLookupManagerCYKPlus(parser, cellColl)
  , m_dictionary(dictionary)
  , m_dbWrapper(dbWrapper)
  , m_inputFactorsVec(inputFactorsVec)
  , m_outputFactorsVec(outputFactorsVec)
{
  UTIL_THROW_IF2(m_expandableDottedRuleListVec.size() != 0,
                 "Dotted rule collection not correctly initialized");

  size_t sourceSize = parser.GetSize();
  m_expandableDottedRuleListVec.resize(sourceSize);
  m_input_default_nonterminal = parser.options()->syntax.input_default_non_terminal;

  for (size_t ind = 0; ind < m_expandableDottedRuleListVec.size(); ++ind) {
    DottedRuleOnDisk *initDottedRule = new DottedRuleOnDisk(m_dbWrapper.GetRootSourceNode());

    DottedRuleStackOnDisk *processedStack = new DottedRuleStackOnDisk(sourceSize - ind + 1);
    processedStack->Add(0, initDottedRule); // init rule. stores the top node in tree

    m_expandableDottedRuleListVec[ind] = processedStack;
  }
}

ChartRuleLookupManagerOnDisk::~ChartRuleLookupManagerOnDisk()
{
  // not needed any more due to the switch to shared pointers
  // std::map<uint64_t, TargetPhraseCollection::shared_ptr >::const_iterator iterCache;
  // for (iterCache = m_cache.begin(); iterCache != m_cache.end(); ++iterCache) {
  //   iterCache->second.reset();
  // }
  // m_cache.clear();

  RemoveAllInColl(m_expandableDottedRuleListVec);
  RemoveAllInColl(m_sourcePhraseNode);
}

void ChartRuleLookupManagerOnDisk::GetChartRuleCollection(
  const InputPath &inputPath,
  size_t lastPos,
  ChartParserCallback &outColl)
{
  const StaticData &staticData = StaticData::Instance();
  // const Word &defaultSourceNonTerm = staticData.GetInputDefaultNonTerminal();
  const Range &range = inputPath.GetWordsRange();

  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();

  // MAIN LOOP. create list of nodes of target phrases
  DottedRuleStackOnDisk &expandableDottedRuleList = *m_expandableDottedRuleListVec[range.GetStartPos()];

  // sort save nodes so only do nodes with most counts
  expandableDottedRuleList.SortSavedNodes();

  const DottedRuleStackOnDisk::SavedNodeColl &savedNodeColl = expandableDottedRuleList.GetSavedNodeColl();
  //cerr << "savedNodeColl=" << savedNodeColl.size() << " ";

  const ChartCellLabel &sourceWordLabel = GetSourceAt(absEndPos);

  for (size_t ind = 0; ind < (savedNodeColl.size()) ; ++ind) {
    const SavedNodeOnDisk &savedNode = *savedNodeColl[ind];

    const DottedRuleOnDisk &prevDottedRule = savedNode.GetDottedRule();
    const OnDiskPt::PhraseNode &prevNode = prevDottedRule.GetLastNode();
    size_t startPos = prevDottedRule.IsRoot() ? range.GetStartPos() : prevDottedRule.GetWordsRange().GetEndPos() + 1;

    // search for terminal symbol
    if (startPos == absEndPos) {
      OnDiskPt::Word *sourceWordBerkeleyDb = m_dictionary.ConvertFromMoses(m_dbWrapper, m_inputFactorsVec, sourceWordLabel.GetLabel());

      if (sourceWordBerkeleyDb != NULL) {
        const OnDiskPt::PhraseNode *node = prevNode.GetChild(*sourceWordBerkeleyDb, m_dbWrapper);
        if (node != NULL) {
          // TODO figure out why source word is needed from node, not from sentence
          // prob to do with factors or non-term
          //const Word &sourceWord = node->GetSourceWord();
          DottedRuleOnDisk *dottedRule = new DottedRuleOnDisk(*node, sourceWordLabel, prevDottedRule);
          expandableDottedRuleList.Add(relEndPos+1, dottedRule);

          // cache for cleanup
          m_sourcePhraseNode.push_back(node);
        }

        delete sourceWordBerkeleyDb;
      }
    }

    // search for non-terminals
    size_t endPos, stackInd;
    if (startPos > absEndPos)
      continue;
    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
      // start.
      endPos = absEndPos - 1;
      stackInd = relEndPos;
    } else {
      endPos = absEndPos;
      stackInd = relEndPos + 1;
    }

    // get target nonterminals in this span from chart
    const ChartCellLabelSet &chartNonTermSet =
      GetTargetLabelSet(startPos, endPos);

    //const Word &defaultSourceNonTerm = staticData.GetInputDefaultNonTerminal()
    //                                   ,&defaultTargetNonTerm = staticData.GetOutputDefaultNonTerminal();

    // go through each SOURCE lhs
    const NonTerminalSet &sourceLHSSet = GetParser().GetInputPath(startPos, endPos).GetNonTerminalSet();

    NonTerminalSet::const_iterator iterSourceLHS;
    for (iterSourceLHS = sourceLHSSet.begin(); iterSourceLHS != sourceLHSSet.end(); ++iterSourceLHS) {
      const Word &sourceLHS = *iterSourceLHS;

      OnDiskPt::Word *sourceLHSBerkeleyDb = m_dictionary.ConvertFromMoses(m_dbWrapper, m_inputFactorsVec, sourceLHS);

      if (sourceLHSBerkeleyDb == NULL) {
        delete sourceLHSBerkeleyDb;
        continue; // vocab not in pt. node definately won't be in there
      }

      const OnDiskPt::PhraseNode *sourceNode = prevNode.GetChild(*sourceLHSBerkeleyDb, m_dbWrapper);
      delete sourceLHSBerkeleyDb;

      if (sourceNode == NULL)
        continue; // didn't find source node

      // go through each TARGET lhs
      ChartCellLabelSet::const_iterator iterChartNonTerm;
      for (iterChartNonTerm = chartNonTermSet.begin(); iterChartNonTerm != chartNonTermSet.end(); ++iterChartNonTerm) {
        if (*iterChartNonTerm == NULL) {
          continue;
        }
        const ChartCellLabel &cellLabel = **iterChartNonTerm;

        bool doSearch = true;
        if (m_dictionary.m_maxSpanDefault != NOT_FOUND) {
          // for Hieu's source syntax

          bool isSourceSyntaxNonTerm = sourceLHS != m_input_default_nonterminal; // defaultSourceNonTerm;
          size_t nonTermNumWordsCovered = endPos - startPos + 1;

          doSearch = isSourceSyntaxNonTerm ?
                     nonTermNumWordsCovered <=  m_dictionary.m_maxSpanLabelled :
                     nonTermNumWordsCovered <= m_dictionary.m_maxSpanDefault;

        }

        if (doSearch) {

          OnDiskPt::Word *chartNonTermBerkeleyDb = m_dictionary.ConvertFromMoses(m_dbWrapper, m_outputFactorsVec, cellLabel.GetLabel());

          if (chartNonTermBerkeleyDb == NULL)
            continue;

          const OnDiskPt::PhraseNode *node = sourceNode->GetChild(*chartNonTermBerkeleyDb, m_dbWrapper);
          delete chartNonTermBerkeleyDb;

          if (node == NULL)
            continue;

          // found matching entry
          //const Word &sourceWord = node->GetSourceWord();
          DottedRuleOnDisk *dottedRule = new DottedRuleOnDisk(*node, cellLabel, prevDottedRule);
          expandableDottedRuleList.Add(stackInd, dottedRule);

          m_sourcePhraseNode.push_back(node);
        }
      } // for (iterChartNonTerm

      delete sourceNode;

    } // for (iterLabelListf

    // return list of target phrases
    DottedRuleCollOnDisk &nodes = expandableDottedRuleList.Get(relEndPos + 1);

    // source LHS
    DottedRuleCollOnDisk::const_iterator iterDottedRuleColl;
    for (iterDottedRuleColl = nodes.begin(); iterDottedRuleColl != nodes.end(); ++iterDottedRuleColl) {
      // node of last source word
      const DottedRuleOnDisk &prevDottedRule = **iterDottedRuleColl;
      if (prevDottedRule.Done())
        continue;
      prevDottedRule.Done(true);

      const OnDiskPt::PhraseNode &prevNode = prevDottedRule.GetLastNode();

      //get node for each source LHS
      const NonTerminalSet &lhsSet = GetParser().GetInputPath(range.GetStartPos(), range.GetEndPos()).GetNonTerminalSet();
      NonTerminalSet::const_iterator iterLabelSet;
      for (iterLabelSet = lhsSet.begin(); iterLabelSet != lhsSet.end(); ++iterLabelSet) {
        const Word &sourceLHS = *iterLabelSet;

        OnDiskPt::Word *sourceLHSBerkeleyDb = m_dictionary.ConvertFromMoses(m_dbWrapper, m_inputFactorsVec, sourceLHS);
        if (sourceLHSBerkeleyDb == NULL)
          continue;

        TargetPhraseCollection::shared_ptr targetPhraseCollection;
        const OnDiskPt::PhraseNode *node
        = prevNode.GetChild(*sourceLHSBerkeleyDb, m_dbWrapper);
        if (node) {
          uint64_t tpCollFilePos = node->GetValue();
          std::map<uint64_t, TargetPhraseCollection::shared_ptr >::const_iterator iterCache = m_cache.find(tpCollFilePos);
          if (iterCache == m_cache.end()) {

            OnDiskPt::TargetPhraseCollection::shared_ptr tpcollBerkeleyDb
            = node->GetTargetPhraseCollection(m_dictionary.GetTableLimit(), m_dbWrapper);

            std::vector<float> weightT = staticData.GetWeights(&m_dictionary);
            targetPhraseCollection
            = m_dictionary.ConvertToMoses(tpcollBerkeleyDb
                                          ,m_inputFactorsVec
                                          ,m_outputFactorsVec
                                          ,m_dictionary
                                          ,weightT
                                          ,m_dbWrapper.GetVocab()
                                          ,true);

            tpcollBerkeleyDb.reset();
            m_cache[tpCollFilePos] = targetPhraseCollection;
          } else {
            // just get out of cache
            targetPhraseCollection = iterCache->second;
          }

          UTIL_THROW_IF2(targetPhraseCollection == NULL, "Error");
          if (!targetPhraseCollection->IsEmpty()) {
            AddCompletedRule(prevDottedRule, *targetPhraseCollection,
                             range, outColl);
          }

        } // if (node)

        delete node;
        delete sourceLHSBerkeleyDb;
      }
    }
  } // for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)

  //cerr << numDerivations << " ";
}

} // namespace Moses
