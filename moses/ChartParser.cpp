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

#include "ChartParser.h"
#include "ChartParserCallback.h"
#include "ChartRuleLookupManager.h"
#include "DummyScoreProducers.h"
#include "StaticData.h"
#include "TreeInput.h"

using namespace std;
using namespace Moses;

namespace Moses
{
extern bool g_debug;

ChartParserUnknown::ChartParserUnknown(const TranslationSystem &system) : m_system(system) {}

ChartParserUnknown::~ChartParserUnknown() {
  RemoveAllInColl(m_unksrcs);
  RemoveAllInColl(m_cacheTargetPhraseCollection);
}

void ChartParserUnknown::Process(const Word &sourceWord, const WordsRange &range, ChartParserCallback &to) {
  // unknown word, add as trans opt
  const StaticData &staticData = StaticData::Instance();
  const UnknownWordPenaltyProducer *unknownWordPenaltyProducer = m_system.GetUnknownWordPenaltyProducer();
  vector<float> wordPenaltyScore(1, -0.434294482); // TODO what is this number?
  
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
  
  Phrase* unksrc = new Phrase(1);
  unksrc->AddWord() = sourceWord;
  m_unksrcs.push_back(unksrc);
  
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
      TargetPhrase *targetPhrase = new TargetPhrase();
      Word &targetWord = targetPhrase->AddWord();
      targetWord.CreateUnknownWord(sourceWord);
      
      // scores
      vector<float> unknownScore(1, FloorScore(TransformScore(prob)));
      
      //targetPhrase->SetScore();
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetScore(m_system.GetWordPenaltyProducer(), wordPenaltyScore);
      targetPhrase->SetSourcePhrase(*unksrc);
      targetPhrase->SetTargetLHS(targetLHS);
      
      // chart rule
      to.AddPhraseOOV(*targetPhrase, m_cacheTargetPhraseCollection, range);
    } // for (iterLHS
  } else {
    // drop source word. create blank trans opt
    vector<float> unknownScore(1, FloorScore(-numeric_limits<float>::infinity()));
    
    TargetPhrase *targetPhrase = new TargetPhrase();
    // loop
    const UnknownLHSList &lhsList = staticData.GetUnknownLHS();
    UnknownLHSList::const_iterator iterLHS;
    for (iterLHS = lhsList.begin(); iterLHS != lhsList.end(); ++iterLHS) {
      const string &targetLHSStr = iterLHS->first;
      //float prob = iterLHS->second;
      
      Word targetLHS(true);
      targetLHS.CreateFromString(Output, staticData.GetOutputFactorOrder(), targetLHSStr, true);
      CHECK(targetLHS.GetFactor(0) != NULL);
      
      targetPhrase->SetSourcePhrase(*unksrc);
      targetPhrase->SetScore(unknownWordPenaltyProducer, unknownScore);
      targetPhrase->SetTargetLHS(targetLHS);
      
      // chart rule
      to.AddPhraseOOV(*targetPhrase, m_cacheTargetPhraseCollection, range);
    }
  }
}

ChartParser::ChartParser(InputType const &source, const TranslationSystem &system, ChartCellCollectionBase &cells) : 
  m_unknown(system),
  m_decodeGraphList(system.GetDecodeGraphs()),
  m_source(source) {
  system.InitializeBeforeSentenceProcessing(source);
  const std::vector<PhraseDictionaryFeature*> &dictionaries = system.GetPhraseDictionaries();
  m_ruleLookupManagers.reserve(dictionaries.size());
  for (std::vector<PhraseDictionaryFeature*>::const_iterator p = dictionaries.begin();
       p != dictionaries.end(); ++p) {
    PhraseDictionaryFeature *pdf = *p;
    const PhraseDictionary *dict = pdf->GetDictionary();
    PhraseDictionary *nonConstDict = const_cast<PhraseDictionary*>(dict);
    m_ruleLookupManagers.push_back(nonConstDict->CreateRuleLookupManager(source, cells));
  }
}

ChartParser::~ChartParser() {
  RemoveAllInColl(m_ruleLookupManagers);
}

void ChartParser::Create(const WordsRange &wordsRange, ChartParserCallback &to) {
  assert(m_decodeGraphList.size() == m_ruleLookupManagers.size());
   
  std::vector <DecodeGraph*>::const_iterator iterDecodeGraph;
  std::vector <ChartRuleLookupManager*>::const_iterator iterRuleLookupManagers = m_ruleLookupManagers.begin();
  for (iterDecodeGraph = m_decodeGraphList.begin(); iterDecodeGraph != m_decodeGraphList.end(); ++iterDecodeGraph, ++iterRuleLookupManagers) {
    const DecodeGraph &decodeGraph = **iterDecodeGraph;
    assert(decodeGraph.GetSize() == 1);
    ChartRuleLookupManager &ruleLookupManager = **iterRuleLookupManagers;
    size_t maxSpan = decodeGraph.GetMaxChartSpan();
    if (maxSpan == 0 || wordsRange.GetNumWordsCovered() <= maxSpan) {
      ruleLookupManager.GetChartRuleCollection(wordsRange, to);
    }
  }
  
  if (wordsRange.GetNumWordsCovered() == 1 && wordsRange.GetStartPos() != 0 && wordsRange.GetStartPos() != m_source.GetSize()-1) {
    bool alwaysCreateDirectTranslationOption = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
    if (to.Empty() || alwaysCreateDirectTranslationOption) {
      // create unknown words for 1 word coverage where we don't have any trans options
      const Word &sourceWord = m_source.GetWord(wordsRange.GetStartPos());
      m_unknown.Process(sourceWord, wordsRange, to);
    }
  }  
}
 
} // namespace Moses
