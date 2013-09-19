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

#include "ChartRuleLookupManagerMemory.h"
#include "ChartRuleLookupManagerMemoryMBOT.h"

#include "PhraseDictionaryMBOT.h"
#include "InputType.h"
#include "ChartTranslationOptionList.h"
#include "CellCollection.h"
#include "DotChartInMemory.h"
#include "StaticData.h"
#include "NonTerminal.h"
#include "ChartCellCollection.h"
#include "ChartTranslationOptionCollection.h"

namespace Moses
{

ChartRuleLookupManagerMemoryMBOT::ChartRuleLookupManagerMemoryMBOT(
  const InputType &src,
  const ChartCellCollection &cellColl,
  const PhraseDictionaryMBOT &ruleTable)
  : ChartRuleLookupManagerMemory(src, cellColl, ruleTable)
  , m_mbotRuleTable(ruleTable)
{

  //std::cout << "BEGIN CREATING DOTTED RULES FOR MBOT" << std::endl;

  CHECK(m_mbotDottedRuleColls.size() == 0);
  size_t sourceSize = src.GetSize();
  m_mbotDottedRuleColls.resize(sourceSize);

  const PhraseDictionaryNodeMBOT &rootNode = m_mbotRuleTable.GetRootNodeMBOT();

  //std::cout << "Found root node : " << *rootNode << std::endl;

  for (size_t ind = 0; ind < m_mbotDottedRuleColls.size(); ++ind) {
#ifdef USE_BOOST_POOL
    DottedRuleInMemoryMBOT *initDottedRule = m_mbotDottedRulePool.malloc();
    new (initDottedRule) DottedRuleInMemoryMBOT(rootNode);
#else
    DottedRuleInMemoryMBOT *initDottedRule = new DottedRuleInMemoryMBOT(rootNode);
    //std::cout << "INI DOTTED RULE NODE Adress : "<< &(initDottedRule->GetLastNode()) << std::endl;
#endif

    DottedRuleCollMBOT *dottedRuleColl = new DottedRuleCollMBOT(sourceSize - ind + 1);
    dottedRuleColl->Add(0, initDottedRule); // init rule. stores the top node in tree

    m_mbotDottedRuleColls[ind] = dottedRuleColl;

    //std::cout << "CRLMMM Constructor size of rule coll : " <<  m_mbotDottedRuleColls.size() << std::endl;
  }
}

ChartRuleLookupManagerMemoryMBOT::~ChartRuleLookupManagerMemoryMBOT()
{
    //std::cout << "ChartRuleLookupManagerMemoryMBOT::~ChartRuleLookupManagerMemoryMBOT()" << std::endl;
  RemoveAllInColl(m_mbotDottedRuleColls);
}

void ChartRuleLookupManagerMemoryMBOT::GetChartRuleCollection(
  const WordsRange &range,
  bool adhereTableLimit,
  ChartTranslationOptionList &outColl)
{
   //std::cout << "CRLMMM : Getting Chart Rule Collection " << std::endl;
  //int debugLevel = 1;
  size_t relEndPos = range.GetEndPos() - range.GetStartPos();
  size_t absEndPos = range.GetEndPos();
  //f+n : inserted for testing
  //if(debugLevel == 1 || debugLevel == 2)
  //{

    //std::cout << "ENTERS FUNCTION" << std::endl;
    //std::cout << "relEndPos " << relEndPos << std::endl;
    //std::cout << "absEndPos " << absEndPos << std::endl;

  // MAIN LOOP. create list of nodes of target phrases

  // get list of all rules that apply to spans at same starting position
  DottedRuleCollMBOT &dottedRuleCol = *m_mbotDottedRuleColls[range.GetStartPos()];
  //std::cout << "CRLMMM : Size of Collection " << dottedRuleCol.GetSizeMBOT() << " " << &dottedRuleCol << std::endl;
  const DottedRuleListMBOT &expandableDottedRuleList = dottedRuleCol.GetExpandableDottedRuleListMBOT();
  //std::cout << "CRLMMM : Size of Expandable list " << expandableDottedRuleList.size() << " " << &expandableDottedRuleList << std::endl;

   //cast to ChartCellLabelMBOT

  //Get input words : strings
  const ChartCellLabelMBOT &sourceWordLabel = GetCellCollection().GetMBOT(WordsRange(absEndPos, absEndPos)).GetSourceWordLabel();
  //std::cerr << "MBOT SOURCE LABEL AT BEGINING : " << absEndPos << " : " << absEndPos << " : " << sourceWordLabel.GetLabelMBOT().size() << sourceWordLabel.GetLabelMBOT().front() << std::endl;


  // loop through the rules
  // (note that expandableDottedRuleList can be expa    nded as the loop runs
  //  through calls to ExtendPartialRuleApplication())
  for (size_t ind = 0; ind < expandableDottedRuleList.size(); ++ind) {
  //std::cout << "Looping through expandableDottedRuleList" << ind << expandableDottedRuleList.size() << std::endl;
    //f+n : inserted for testing
    //if(debugLevel == 2)
    //std::cout << "ind " << ind << std::endl;
    // rule we are about to extend
    const DottedRuleInMemoryMBOT * prevDottedRule = expandableDottedRuleList[ind];
    //std::cout << "PREVIOUS DOTTED RULE IN EXPANDABLE RULE LIST : " << *prevDottedRule << std::endl;
    //std::cout << *prevDottedRule << std::endl;
    //std::cout << "Adress of previous dotted rule " << prevDottedRule << std::endl;

    // we will now try to extend it, starting after where it ended
    size_t startPos = prevDottedRule->IsRootMBOT()
                    ? range.GetStartPos()
                    : prevDottedRule->GetWordsRangeMBOT().GetEndPos() + 1;

    //f+n : inserted for testing
    //if(debugLevel == 1)
    //std::cout << "BEFORE IFS : Start " << startPos << " : " << absEndPos <<  std::endl;
    // search for terminal symbol
    // (if only one more word position needs to be covered)
    if (startPos == absEndPos) {
    //std::cout << "1 :1 : Only one position covered : " << std::endl;

      // look up in rule dictionary, if the current rule can be extended
      // with the source word in the last position
      //take first element of MBOT label for source word

      //source word label is MBOT
      //std::cout << "Getting source label " << std::endl;
      //BEWARE : source word label may be empty!!!
      CHECK(sourceWordLabel.GetLabelMBOT().size() == 1);
      const Word &sourceWord = sourceWordLabel.GetLabelMBOT().front();
      //std::cout << "1 :1 : Getting source WORD :" << sourceWord << std::endl;
      //std::cout << "Source label here " << sourceWord  << std::endl;

      //FB : Found the right Node !!!
      const PhraseDictionaryNodeMBOT *node = prevDottedRule->GetLastNode().GetChildMBOT(sourceWord);
      //std::cout << "Adress of Node" << node << std::endl;
      //abort();

       // f+n : inserted for testing
      //if(debugLevel == 1)
	//{std::cout << "sourceWord " << sourceWord << std::endl;}

     // if we found a new rule -> create it and add it to the list
      if (node != NULL) {
      //std::cout << "CRLM : Node is not NULL : " << node << std::endl;
	//if(debugLevel == 3)
	  //{std::cout << "node " << node->GetTargetPhraseCollection() << std::endl;}
			// create the rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemoryMBOT *dottedRule = m_dottedRulePool.malloc();
        new (dottedRule) DottedRuleInMemoryMBOT(*node, sourceWordLabel,
                                            prevDottedRule);
       // f+n : inserted for testing
	//if(debugLevel == 1)
	  //{std::cout << "IFDEF" << std::endl;
	    //std::cout << "\tdottedRule " << dottedRule->GetWordsRange() << "=" << dottedRule->GetSourceWord() << std::endl;
	    //std::cout << "\tsourceWordLabel " << sourceWord << std::endl;
	    //std::cout << "\tprevDottedRule " << prevDottedRule << std::endl;}
#else
        //std::cout << "MAKING DOTTED RULE IN MEMORY" << std::endl;
        DottedRuleInMemoryMBOT *dottedRule = new DottedRuleInMemoryMBOT(*node,
                                                                sourceWordLabel,
                                                                *prevDottedRule);
        //std::cout << "CRLM : " << (*dottedRule) << std::endl;
       // f+n : inserted for testing
	//if(debugLevel == 1)
	  //{std::cout << "ELSE" << std::endl;

	    //std::cout << "\tdottedRule " << dottedRule->GetWordsRange() << " = " << dottedRule->GetSourceWord() << " " <<  prevDottedRule <<  std::endl;
	    //std::cout << "\tsourceWordLabel " << sourceWord << std::endl;
	    //std::cout << "\tprevDottedRule " << prevDottedRule << std::endl;}
#endif
        //std::cout << "CRLMMM : Found Dotted Rule "<< *dottedRule << std::endl;
        //std::cout << "Adress of found rule "<< dottedRule << std::endl;
        dottedRuleCol.Add(relEndPos+1, dottedRule);
       // f+n : inserted for testing
	//if(debugLevel == 1)
	  //std::cout << "dottedRule " << dottedRule->GetWordsRange() << " = " << dottedRule->GetSourceWord() << " " <<   prevDottedRule <<  std::endl;}
      }
    }

    //CONTINUE HERE !!!

    // search for non-terminals
    size_t endPos, stackInd;

    // span is already complete covered? nothing can be done
    if (startPos > absEndPos){
       //std::cout << "Rule Completed !" << std::endl;
      continue;}


    //FB : for non-terminals span is > 1
    else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos()) {
    //std::cout << " n : n : Processing Root of Prefix Tree..." << std::endl;
      // We're at the root of the prefix tree so won't try to cover the full
      // span (i.e. we don't allow non-lexical unary rules).  However, we need
      // to match non-unary rules that begin with a non-terminal child, so we
      // do that in two steps: during this iteration we search for non-terminals
      // that cover all but the last source word in the span (there won't
      // already be running nodes for these because that would have required a
      // non-lexical unary rule match for an earlier span).  Any matches will
      // result in running nodes being appended to the list and on subsequent
      // iterations (for this same span), we'll extend them to cover the final
      // word.
      endPos = absEndPos - 1;
      stackInd = relEndPos;
    }
    else
    {
      endPos = absEndPos;
      stackInd = relEndPos + 1;
    }

    //std::cout << "EXTEND partial rule applications" << std::endl;
    ExtendPartialRuleApplicationMBOT(*prevDottedRule, startPos, endPos, stackInd,
                                 dottedRuleCol);
  }

  //std::cout << "Searching for First dotted rules  : " << relEndPos + 1 << std::endl;

  // list of rules that that cover the entire span
  DottedRuleListMBOT &rules = dottedRuleCol.GetMBOT(relEndPos + 1);
  //std::cout << "n : n : Size of DottedRuleList " << rules.size () <<  std::endl;
   //std::cout << "ADDRESS of DottedRuleList " << &rules <<  std::endl;
  // look up target sides for the rules
  size_t rulesLimit = StaticData::Instance().GetRuleLimit();
  DottedRuleListMBOT::const_iterator iterRule;

  //  for (int i=0; i<rules.size(); i++)
  // {
  //    const DottedRuleInMemoryMBOT *dottedRuleMBOT = rules[i];
  //f*n : inserted for testing
  //std::cout << "BEGIN LOOP OVER RULE COLLECTION" << std::endl;
        for (iterRule = rules.begin(); iterRule != rules.end(); ++iterRule) {
      const DottedRuleInMemoryMBOT &dottedRuleMBOT = **iterRule;

    //f+n : inserted for testing
    //std::cout << "Complete rule adress: " << dottedRuleMBOT  << std::endl;
    //std :: cout << "Rule outted "<< std::endl;

    const PhraseDictionaryNodeMBOT &node = dottedRuleMBOT.GetLastNode();


    //std::cout << "Node returned" << node << std::endl;

    // f+n : inserted for testing
    //if (debugLevel == 3)
      //{std::cout << "node " << node << std::endl;}
    // look up target sides
    const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollectionMBOT();
    //std::cout << "TRYING TO GET TARGET PHRASE COLLECTION"<< std::endl;
    //const TargetPhraseMBOT targetPhrase = **((*targetPhraseCollection).begin());

    //std::cout << "Target Phrase Collection getted "<< targetPhrase << std::endl;

    //const DottedRuleInMemoryMBOT * dottedRule = *iterRule;
    //std::cout << "n : n : Found Rule in Collection" << dottedRuleMBOT << std::endl;


     // add the fully expanded rule (with lexical target side)
    if (targetPhraseCollection != NULL) {
    //std::cout << "TARGET PHRASE COLLECTION IS NOT EMPTY!" << std::endl;
    //std::cout << "Size of collection : " << targetPhraseCollection->GetSize() << std::endl;
      // f+n : inserted for testing
      //if (debugLevel == 2)
	//{std::cout << "targetPhraseCollection " << node.GetTargetPhraseCollection() << std::endl;}

     DottedRuleInMemoryMBOT * dottedRuleNotConst = const_cast<DottedRuleInMemoryMBOT*>(&dottedRuleMBOT);
     DottedRuleMBOT *dottedRuleForColl = static_cast<DottedRuleMBOT*>(dottedRuleNotConst);
     //std::cout << "n : n : ADD TO COLLECTION : " << *dottedRuleForColl << std::endl;

      outColl.AddMBOT(*targetPhraseCollection, *dottedRuleForColl,
                  GetCellCollection(), adhereTableLimit, rulesLimit);
    }
  }
  //std::cout << "END LOOP OVER RULE COLLECTION" << std::endl;
  dottedRuleCol.Clear(relEndPos+1);
  outColl.CreateChartRules(rulesLimit);
}

// Given a partial rule application ending at startPos-1 and given the sets of
// source and target non-terminals covering the span [startPos, endPos],
// determines the full or partial rule applications that can be produced through
// extending the current rule application by a single non-terminal.
void ChartRuleLookupManagerMemoryMBOT::ExtendPartialRuleApplicationMBOT(
  const DottedRuleInMemoryMBOT &prevDottedRule,
  size_t startPos,
  size_t endPos,
  size_t stackInd,
  DottedRuleCollMBOT & dottedRuleColl)
{
    //std::cout << "Extending partial rule application : " << startPos << " : " << endPos << ": Stack ind : " << stackInd << std::endl;
    //f+n: inserted for testing
    int debugLevel = 1;

    const NonTerminalSet &sourceNonTerms = GetSentence().GetLabelSet(startPos, endPos);
    NonTerminalSet allSourceNonTerms;
    NonTerminalSet::iterator itr_nonTermMap;

    //Fabienne Braune : TODO : make input factors correctly
    std::vector<size_t> inputFactors;
    if(StaticData::Instance().GetSourceNonTerminals().size() > 0)
    {
    	 std::cerr << "Number of input non-terminals : " << StaticData::Instance().GetSourceNonTerminals().size() << std::endl;
    	 std::vector<std::string>::const_iterator itr_source_terms;
    	 for(itr_source_terms = StaticData::Instance().GetSourceNonTerminals().begin();itr_source_terms != StaticData::Instance().GetSourceNonTerminals().end();itr_source_terms++)
    	 {
    		 Word sourceNonTerm;
    		 sourceNonTerm.CreateFromString(Input,inputFactors, *itr_source_terms, true);
    		 allSourceNonTerms.insert(sourceNonTerm);
		}

    	 //also insert labels of current sentence to make sure we have everything
    	 for(itr_nonTermMap = sourceNonTerms.begin(); itr_nonTermMap != sourceNonTerms.end(); itr_nonTermMap++)
    	 {
    	     Word sourceWord =  *itr_nonTermMap;
    	     allSourceNonTerms.insert(sourceWord);
    	 }
    }


  // target non-terminal labels for the remainder
  //cast to ChartCellLabelMBOT
  // changed form ChartCellMBOT mbotCell (destruction)
    //std::cout << "GETTING TARGET NON TERMS 666 : "<< std::endl;
    const ChartCellLabelSetMBOT &targetNonTerms =
    GetCellCollection().GetMBOT(WordsRange(startPos, endPos)).GetTargetLabelSet();

    //std::cout << "SIZE OF OBTAINED TARGET NON TERMS : " << targetNonTerms.GetSizeMBOT() << std::endl;

    //new: inserted for testing
    //std::cout << "Chart Cells "<< startPos << ".." << endPos << " " << GetCellCollection().Get(WordsRange(startPos, endPos)) << std::endl;
    //std::cout << "Chart Cells displayed !" << std::endl;

   //f+n : inserted for testing
  //if(debugLevel == 1)
  //{
    //std::cout << "Target non-terminals" << startPos << "..." << endPos << " : " << std::endl;
    ChartCellLabelSetMBOT::const_iterator my_iter_target;
    for(my_iter_target = targetNonTerms.begin(); my_iter_target != targetNonTerms.end(); my_iter_target++)
    {
      //std::cout << "in for loop" << std::endl;
      const ChartCellLabelMBOT &cellLabel = *my_iter_target;
      //std::cout << "EXTEND : targetCellLabel :" << cellLabel.GetLabelMBOT().front() << std::endl;
    }
    //}
  //}

//f+n: inserted for testing
  // if(debugLevel == 1)
  // {std::cout << "Source non terms " << sourceNonTerms << std::endl;
    //std::cout << "Target non terms " << targetNonTerms  << std::endl;

  // note where it was found in the prefix tree of the rule dictionary
  const PhraseDictionaryNodeMBOT &node = prevDottedRule.GetLastNode();

  //std::cout << "Last node getted : " << &node << std::endl;

  const PhraseDictionaryNodeMBOT::NonTerminalMapMBOT & nonTermMap =
    node.GetNonTerminalMapMBOT();

    //FB ; inserted for testing : print MAP
    //std::cout << "SIZE OF MAP : " << nonTermMap.size() << std::endl;

    /*PhraseDictionaryNodeMBOT::NonTerminalMapMBOT::const_iterator itr_nonTermMapMBOT;
    for(itr_nonTermMapMBOT = nonTermMap.begin(); itr_nonTermMapMBOT != nonTermMap.end(); itr_nonTermMap++)
    {
        std::pair<Word, std::vector<Word> > nonTermMapPair = (*itr_nonTermMapMBOT).first;
        std::cout << "KEY: " << nonTermMapPair.first << " ";

        std::vector<Word> values = nonTermMapPair.second;
        std::vector<Word>::iterator itr_values;

        int counter = 0;
        for(itr_values = values.begin(); itr_values != values.end(); itr_values++)
        {
            std::cout << "VALUES : " << *itr_values << "("<< ++counter << ")" << std::endl;
        }
        std::cout << "OUT OF LOOP      " << std::endl;

    }*/


     //std::cout << "Non Term map getted" << std::endl;

  //f+n: inserted for testing
  //if(debugLevel == 1)
  //{std::cout << "Non Term map " << nonTermMap << std::endl;}

  const size_t numChildren = nonTermMap.size();
  //std::cout << "CHILDREN : " << numChildren << std::endl;


  if (numChildren == 0) {
    //std::cout << "Before Return" << std::endl;
    return;
  }

  //source non terms
  //Fabienne Braune : If we implement the no-match semantics, we iterate over allSourceNonTerms
  //Otherwise, we iterate over
  //if(StaticData::Instance().GetSourceNonTerminals().size() > 0)
  //{}
  //else
  //{}

  //TODO : if source label option is on then use source labels, otherwise do not!
  const size_t numSourceNonTerms = sourceNonTerms.size();
  const size_t numTargetNonTerms = targetNonTerms.GetSizeMBOT();
  //std::cout << "NTNT : " << numTargetNonTerms << std::endl;
  const size_t numCombinations = numSourceNonTerms * numTargetNonTerms;

  //f+n: inserted for testing
  //if(debugLevel == 2)
  //{std::cout << "EXTnumSourceNonTerms " << numSourceNonTerms << std::endl;
  //  std::cout << "EXTnumTargetNonTerms " << numTargetNonTerms  << std::endl;
  //  std::cout << "EXTnumCombinations " << numCombinations  << std::endl;}

  // We can search by either:
  //   1. Enumerating all possible source-target NT pairs that are valid for
  //      the span and then searching for matching children in the node,
  // or
  //   2. Iterating over all the NT children in the node, searching
  //      for each source and target NT in the span's sets.
  // We'll do whichever minimises the number of lookups:
  if (numCombinations <= numChildren*2) {

		// loop over possible source non-terminal labels (as found in input tree)
    NonTerminalSet::const_iterator p = sourceNonTerms.begin();
    NonTerminalSet::const_iterator sEnd = sourceNonTerms.end();
    for (; p != sEnd; ++p) {
      const Word & sourceNonTerm = *p;
      //std::cerr << "EXTEND : source non terminal : " << sourceNonTerm << std::endl;

      // loop over possible target non-terminal labels (as found in chart)
      ChartCellLabelSetMBOT::const_iterator q = targetNonTerms.begin();
      ChartCellLabelSetMBOT::const_iterator tEnd = targetNonTerms.end();
      for (; q != tEnd; ++q) {
        //std::cout << "In first iteration" << std::endl;
        const ChartCellLabelMBOT &cellLabel = *q;

        //std::vector<Word>::const_iterator itr_labels;
        //for(itr_labels = cellLabel.GetLabelMBOT().begin(); itr_labels != cellLabel.GetLabelMBOT().end(); itr_labels++)
        //{
          //std::cerr << " EXTEND : Cell label mbot : " << *itr_labels << std::endl;
        //}

        //std::cerr << "GETTING CHILD FOR  : " << sourceNonTerm << std::endl;
        // try to match both source and target non-terminal
        const PhraseDictionaryNodeMBOT * child =
          node.GetChild(sourceNonTerm, cellLabel.GetLabelMBOT());

        // nothing found? then we are done
        if (child == NULL) {
          //std::cout << "CHILD NOT FOUND !" << std::endl;
          continue;
        }

        // create new rule
#ifdef USE_BOOST_POOL
        DottedRuleInMemory *rule = m_dottedRulePool.malloc();
        new (rule) DottedRuleInMemoryMBOT(*child, cellLabel, prevDottedRule);
        //std::cout << "EXTEND : New rule found : " << *rule << std::endl;
#else
        DottedRuleInMemoryMBOT *rule = new DottedRuleInMemoryMBOT(*child, cellLabel,
                                                          prevDottedRule);

        //new : put source info into rule
        rule->SetSourceLabel(sourceNonTerm);

        //std::cout << "EXTEND : New created rule : " << *rule << std::endl;
#endif
        //std::cout << "ADDING RULE TO COLLECTION..." << std::endl;
        //std::cerr << " ADDING MBOT RULE TO COLLECTION : " << *rule << std::endl;
        dottedRuleColl.Add(stackInd, rule);
      }
    }
  }
  else
  {
    // loop over possible expansions of the rule
    PhraseDictionaryNodeMBOT::NonTerminalMapMBOT::const_iterator p;
    PhraseDictionaryNodeMBOT::NonTerminalMapMBOT::const_iterator end =
      nonTermMap.end();
    //std::cout << "Iterating over non terminal map :" << std::endl;
    for (p = nonTermMap.begin(); p != end; ++p) {
      // does it match possible source and target non-terminals?
      const PhraseDictionaryNodeMBOT::NonTerminalMapKeyMBOT &key = p->first;
      const Word &sourceNonTerm = key.first;
      //std::cout << "EXTEND : source word "<< sourceNonTerm << std::endl;
      if (sourceNonTerms.find(sourceNonTerm) == sourceNonTerms.end()) {
        //std::cout << "NOT FOUND C������NTINUE"<< std::endl;
        continue;
      }
      //const std::vector<Word> &targetNonTerm = key.second.GetWordVector();
      const std::vector<Word> &targetNonTerm = key.second;
      //std::cout << "EXTEND : corresponding target words : " << targetNonTerm.front() << std::endl;
      const ChartCellLabelMBOT *cellLabel = targetNonTerms.FindMBOT(targetNonTerm);


      if (!cellLabel) {
        //std::cout << "EXTEND : cell label not found, continue"<< std::endl;
        continue;
      }

      // create new rule
      const PhraseDictionaryNodeMBOT &child = p->second;
#ifdef USE_BOOST_POOL
      DottedRuleInMemoryMBOT *rule = m_dottedRulePool.malloc();
      new (rule) DottedRuleInMemoryMBOT(child, *cellLabel, prevDottedRule);
#else
      DottedRuleInMemoryMBOT *rule = new DottedRuleInMemoryMBOT(child, *cellLabel,
                                                        prevDottedRule);
#endif

    //new : put source info into rule
      rule->SetSourceLabel(sourceNonTerm);
      //std::cerr << " ADDING MBOT RULE TO COLLECTION : " << *rule << std::endl;
      dottedRuleColl.Add(stackInd, rule);
    }
  }
}

}  // namespace Moses
