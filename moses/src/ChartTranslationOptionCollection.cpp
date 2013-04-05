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

#include "util/check.hh"
#include "ChartTranslationOptionCollection.h"
#include "ChartCellCollection.h"
#include "InputType.h"
#include "StaticData.h"
#include "DecodeStep.h"
#include "DummyScoreProducers.h"
#include "Util.h"
#include "RuleMap.h"

#include "CellContextScoreProducer.h"


using namespace std;

namespace Moses
{

ChartTranslationOptionCollection::ChartTranslationOptionCollection(InputType const& source
    , const TranslationSystem* system
    , const ChartCellCollection &hypoStackColl
    , const std::vector<ChartRuleLookupManager*> &ruleLookupManagers)
  :m_source(source)
  ,m_system(system)
  ,m_decodeGraphList(system->GetDecodeGraphs())
  ,m_hypoStackColl(hypoStackColl)
  ,m_ruleLookupManagers(ruleLookupManagers)
  ,m_translationOptionList(StaticData::Instance().GetRuleLimit())
{
}

ChartTranslationOptionCollection::~ChartTranslationOptionCollection()
{
  RemoveAllInColl(m_unksrcs);
  RemoveAllInColl(m_cacheTargetPhraseCollection);
}

void ChartTranslationOptionCollection::CreateTranslationOptionsForRange(
  const WordsRange &wordsRange)
{

  //std::cout << "Creating translation options for range : " << wordsRange << std::endl;

  assert(m_decodeGraphList.size() == m_ruleLookupManagers.size());

  m_translationOptionList.Clear();

  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    assert(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    if (maxSpan == 0 || wordsRange.GetNumWordsCovered() <= maxSpan) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, m_translationOptionList);
    }
  }

  CellContextScoreProducer *ccsp = StaticData::Instance().GetCellContextScoreProducer();
  if(ccsp!=NULL){
    //damt hiero : if we use context features then : For each translation option :
        //1. Go through each target phrase and access corresponding source phrase
        //2. Store all targetPhrases for this source phrase
        //3. Store references to targets in map (TargetRepresentation,TargetPhrase*)
        //4. Call vw
        //5. Store new score in each targetPhrase
        //6. Re-estimate score of this translation option
//    #ifdef HAVE_VW
    vector<FactorType> srcFactors;
    srcFactors.push_back(0);
    string nonTermRep = "[X][X]";

    VERBOSE(0, "Computing vw scores for source context : " << m_source << endl);     

    for (size_t i = 0; i < m_translationOptionList.GetSize(); ++i) {

        ChartTranslationOption &transOpt = m_translationOptionList.Get(i);

        //all words ranges should be the same, otherwise crash
        CHECK(transOpt.GetSourceWordsRange() == wordsRange);

        std::string sourceSide = "";
        std::string targetRepresentation;
        TargetPhraseCollection::const_iterator itr_targets;

        //map source to accumulated target phrases
         RuleMap ruleMap;

         //Map target representation to targetPhrase
         std::map<std::string,TargetPhrase*> targetRepMap;

         VERBOSE(5, "Looping over target phrase collection for recomputing feature scores" << endl);

        //Get non-rescored target phrase collection for inserting newly scored target
        for(
            itr_targets = transOpt.GetTargetPhraseCollection().begin();
            itr_targets != transOpt.GetTargetPhraseCollection().end();
            itr_targets++)
        {
            //get source side of rule
            CHECK((**itr_targets).GetSourcePhrase() != NULL);

            //rewrite non-terminals non source side with "[][]"
            for(int i=0; i<(**itr_targets).GetSourcePhrase()->GetSize();i++)
            {
                //replace X by [X][X] for coherence with rule table
                if((**itr_targets).GetSourcePhrase()->GetWord(i).IsNonTerminal())
                {
                    sourceSide += nonTermRep;
                }
                else
                {
                    sourceSide += (**itr_targets).GetSourcePhrase()->GetWord(i).GetString(srcFactors,0);
                }

                if(i<(**itr_targets).GetSourcePhrase()->GetSize() -1)
                {
                    sourceSide += " ";
                }
            }

            //Add parent to source
            std::string parentNonTerm = "[X]";
            sourceSide += " ";
            sourceSide += parentNonTerm;

            int wordCounter = 0;

            //NonTermCounter should stay smaller than nonTermIndexMap
            for(int i=0; i<(**itr_targets).GetSize();i++)
            {
                CHECK(wordCounter < (**itr_targets).GetSize());

                //look for non-terminals
                if((**itr_targets).GetWord(i).IsNonTerminal() == 1)
                {
                    //append non-terminal
                    targetRepresentation += nonTermRep;

                    //Debugging : everything OK in non term index map
                    //for(int i=0; i < 3; i++)
                    //{std::cout << "TEST : Non Term index map : " << i << "=" << ntim[i] << std::endl;}
                    const AlignmentInfo alignInfo = (*itr_targets)->GetAlignmentInfo();
                    AlignmentInfo::const_iterator itr_align;
                    string alignInd;
                    //damt hiero : not nice. TODO : better implementation
                    for(itr_align = alignInfo.begin(); itr_align != alignInfo.end(); itr_align++)
                    {
                        //look for current target
                        if( itr_align->second == wordCounter)
                        {
                            stringstream s;
                            s << itr_align->first;
                            alignInd = s.str();
                        }
                    }
                    targetRepresentation += alignInd;
                }
                else
                {
                    targetRepresentation += (**itr_targets).GetWord(i).GetString(srcFactors,0);
                }
                if(i<(**itr_targets).GetSize() -1)
                {
                    targetRepresentation += " ";
                }
                wordCounter++;
            }

            //add parent label to target
            targetRepresentation += " ";
            targetRepresentation += parentNonTerm;

            VERBOSE(3, "Strings put in rule map : " << sourceSide << "::" << targetRepresentation << endl);
            ruleMap.AddRule(sourceSide,targetRepresentation);
            targetRepMap.insert(std::make_pair(targetRepresentation,*itr_targets));
            targetRepMap.insert(std::make_pair(targetRepresentation,*itr_targets));

            //clean strings
            sourceSide = "";
            targetRepresentation = "";
        }

        RuleMap::const_iterator itr_ruleMap;
        for(itr_ruleMap = ruleMap.begin(); itr_ruleMap != ruleMap.end(); itr_ruleMap++)
        {
            CellContextScoreProducer *ccsp = StaticData::Instance().GetCellContextScoreProducer();
            CHECK(ccsp != NULL);
            VERBOSE(3, "Calling vw for rule : " << itr_ruleMap->first << " : " << itr_ruleMap->second << endl);
            VERBOSE(3, "Calling vw for source context : " << m_source << endl);

            vector<ScoreComponentCollection> scores = ccsp->ScoreRules(
                                                                       transOpt.GetSourceWordsRange().GetStartPos(),
                                                                       transOpt.GetSourceWordsRange().GetEndPos(),
                                                                       itr_ruleMap->first,itr_ruleMap->second,
                                                                       m_source,
                                                                       &targetRepMap
                                                                       );

            //loop over target phrases and add score
            std::vector<ScoreComponentCollection>::const_iterator iterLCSP = scores.begin();
            std::vector<std::string> :: iterator itr_targetRep;
            for (itr_targetRep = (itr_ruleMap->second)->begin() ; itr_targetRep != (itr_ruleMap->second)->end() ; itr_targetRep++) {
                //Find target phrase corresponding to representation
                std::map<std::string,TargetPhrase*> :: iterator itr_rep;
                CHECK(targetRepMap.find(*itr_targetRep) != targetRepMap.end());
                itr_rep = targetRepMap.find(*itr_targetRep);
                VERBOSE(0, "Looking at target phrase : " << *itr_rep->second << std::endl);
		VERBOSE(0, "Target Phrase score vector before adding stateless : " << (itr_rep->second)->GetScoreBreakdown() << std::endl);
                VERBOSE(0, "Target Phrase score before adding stateless : " << (itr_rep->second)->GetFutureScore() << std::endl);
                VERBOSE(0, "Score component collection : " << *iterLCSP << std::endl);
                (itr_rep->second)->AddStatelessScore(*iterLCSP++);
                VERBOSE(0, "Target Phrase score after adding stateless : " << (itr_rep->second)->GetFutureScore() << std::endl);
                }
        }
        //sort target phrase collection again
        transOpt.SortTargetPhrases();

        //NOTE : What happens with the stack vector?
        VERBOSE(0, "Estimate of best score before computing context : " << transOpt.GetEstimateOfBestScore() << std::endl);
        transOpt.CalcEstimateOfBestScore();
        VERBOSE(0, "Estimate of best score after computing context : " << transOpt.GetEstimateOfBestScore() << std::endl);
    }
  }//end of ifs
//    #endif // HAVE_VW

  if (wordsRange.GetNumWordsCovered() == 1 && wordsRange.GetStartPos() != 0 && wordsRange.GetStartPos() != m_source.GetSize()-1) {
    bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
    if (m_translationOptionList.GetSize() == 0 || alwaysCreateDirectTranslationOption) {
      // create unknown words for 1 word coverage where we don't have any trans options
      const Word &sourceWord = m_source.GetWord(wordsRange.GetStartPos());
      ProcessOneUnknownWord(sourceWord, wordsRange);
    }
  }

  //pruning after loading rule table differed here
  m_translationOptionList.ShrinkToLimit();
  m_translationOptionList.ApplyThreshold();
}

//! special handling of ONE unknown words.
void ChartTranslationOptionCollection::ProcessOneUnknownWord(const Word &sourceWord, const WordsRange &range)
{
  // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = m_system->GetUnknownWordPenaltyProducer();
  vector<float> wordPenaltyScore(1, -0.434294482); // TODO what is this number?

  const ChartCell &chartCell = m_hypoStackColl.Get(range);
  const ChartCellLabel &sourceWordLabel = chartCell.GetSourceWordLabel();

  size_t isDigit = 0;
  if (staticData.GetDropUnknown()) {
    const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
    const string &s = f->GetString();
    isDigit = s.find_first_of("0123456789");
    if (isDigit == string::npos)
      isDigit = 0;
    else
      isDigit = 1;
    // modify the starting bitmap
  }

  Phrase* m_unksrc = new Phrase(1);
  m_unksrc->AddWord() = sourceWord;
  m_unksrcs.push_back(m_unksrc);

  //TranslationOption *transOpt;
  if (! staticData.GetDropUnknown() || isDigit) {
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      float prob = iterLHS->second;

      // lhs
      //const Word &sourceLHS = staticData.GetInputDefaultNonTerminal();
      Word targetLHS(true);

      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);

      // add to dictionary
      TargetPhrase *targetPhrase = new TargetPhrase(Output,*m_unksrc);
      TargetPhraseCollection *tpc = new TargetPhraseCollection;
      tpc->Add(targetPhrase);

      m_cacheTargetPhraseCollection.push_back(tpc);
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(sourceWord);

      // scores
      vector<float> unknownScore(1, FloorScore(TransformScore(prob)));

      //targetPhrase->SetScore();
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetScore(m_system->GetWordPenaltyProducer(), wordPenaltyScore);

      //targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetTargetLHS(targetLHS);
      targetPhrase->SetAlignmentInfo("0-0");

      //damt hiero : get cell context score producer and add 0 psdscore
      CellContextScoreProducer *ccsp = StaticData::Instance().GetCellContextScoreProducer();
      if(ccsp != NULL)
      {CHECK(ccsp != NULL);
      targetPhrase->AddStatelessScore(ccsp->ScoreFactory(0));}

      // chart rule
      m_translationOptionList.Add(*tpc, m_emptyStackVec, range);
    } // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));

    TargetPhrase *targetPhrase = new TargetPhrase(Output,*m_unksrc);
    TargetPhraseCollection *tpc = new TargetPhraseCollection;
    tpc->Add(targetPhrase);
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      //float prob = iterLHS->second;

      Word targetLHS(true);
      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);

      m_cacheTargetPhraseCollection.push_back(tpc);
      //damt hiero : source phrase built in constructor of target phrase
      //targetPhrase->SetSourcePhrase(m_unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);

      // chart rule
      m_translationOptionList.Add(*tpc, m_emptyStackVec, range);
    }
  }
}

}  // namespace
