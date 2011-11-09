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

#include "RuleTableLoaderStandard.h"

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "PhraseDictionarySCFG.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "ChartTranslationOptionList.h"
#include "DotChart.h"
#include "FactorCollection.h"

using namespace std;

namespace Moses
{

bool RuleTableLoaderStandard::Load(const std::vector<FactorType> &input
                                , const std::vector<FactorType> &output
                                , std::istream &inStream
                                , const std::vector<float> &weight
                                , size_t /* tableLimit */
                                , const LMList &languageModels
                                , const WordPenaltyProducer* wpProducer
                                , PhraseDictionarySCFG &ruleTable)
{
  PrintUserTime("Start loading new format pt model");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();


  string line;
  size_t count = 0;

  while(getline(inStream, line)) {
    vector<string> tokens;
    vector<float> scoreVector;

    TokenizeMultiCharSeparator(tokens, line , "|||" );

    if (tokens.size() != 4 && tokens.size() != 5) {
      stringstream strme;
      strme << "Syntax error at " << ruleTable.GetFilePath() << ":" << count;
      UserMessage::Add(strme.str());
      abort();
    }

    const string &sourcePhraseString = tokens[0]
               , &targetPhraseString = tokens[1]
               , &scoreString        = tokens[2]
               , &alignString        = tokens[3];

    bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
    if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
      TRACE_ERR( ruleTable.GetFilePath() << ":" << count << ": pt entry contains empty target, skipping\n");
      continue;
    }

    Tokenize<float>(scoreVector, scoreString);
    const size_t numScoreComponents = ruleTable.GetFeature()->GetNumScoreComponents();
    if (scoreVector.size() != numScoreComponents) {
      stringstream strme;
      strme << "Size of scoreVector != number (" << scoreVector.size() << "!="
            << numScoreComponents << ") of score components on line " << count;
      UserMessage::Add(strme.str());
      abort();
    }
    assert(scoreVector.size() == numScoreComponents);

    // parse source & find pt node

    // constituent labels
    Word sourceLHS, targetLHS;

    // source
    Phrase sourcePhrase(Input, 0);
    sourcePhrase.CreateFromStringNewFormat(Input, input, sourcePhraseString, factorDelimiter, sourceLHS);

    // create target phrase obj
    TargetPhrase *targetPhrase = new TargetPhrase(Output);
    targetPhrase->CreateFromStringNewFormat(Output, output, targetPhraseString, factorDelimiter, targetLHS);

    // rest of target phrase
    targetPhrase->SetAlignmentInfo(alignString);
    targetPhrase->SetTargetLHS(targetLHS);
    //targetPhrase->SetDebugOutput(string("New Format pt ") + line);

    // component score, for n-best output
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),TransformScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

    targetPhrase->SetScoreChart(ruleTable.GetFeature(), scoreVector, weight, languageModels,wpProducer);

    TargetPhraseCollection &phraseColl = GetOrCreateTargetPhraseCollection(ruleTable, sourcePhrase, *targetPhrase, sourceLHS);
    phraseColl.Add(targetPhrase);

    count++;
  }

  // sort and prune each target phrase collection
  SortAndPrune(ruleTable);

  return true;
}

}
