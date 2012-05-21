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

#include "RuleTable/LoaderStandard.h"

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "RuleTable/Trie.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "ChartTranslationOptionList.h"
#include "FactorCollection.h"

using namespace std;

namespace Moses
{
bool RuleTableLoaderStandard::Load(const std::vector<FactorType> &input
                                   , const std::vector<FactorType> &output
                                   , std::istream &inStream
                                   , const std::vector<float> &weight
                                   , size_t tableLimit
                                   , const LMList &languageModels
                                   , const WordPenaltyProducer* wpProducer
                                   , RuleTableTrie &ruleTable)
{
  bool ret = Load(MosesFormat
                  ,input, output
                  ,inStream, weight
                  ,tableLimit, languageModels
                  ,wpProducer, ruleTable);
  return ret;

}
  
void ReformatHieroRule(int sourceTarget, string &phrase, map<size_t, pair<size_t, size_t> > &ntAlign)
{
  vector<string> toks;
  Tokenize(toks, phrase, " ");

  for (size_t i = 0; i < toks.size(); ++i)
  {
    string &tok = toks[i];
    size_t tokLen = tok.size();
    if (tok.substr(0, 1) == "[" && tok.substr(tokLen - 1, 1) == "]")
    { // no-term
      vector<string> split = Tokenize(tok, ",");
      CHECK(split.size() == 2);
      
      tok = "[X]" + split[0] + "]";
      size_t coIndex = Scan<size_t>(split[1]);
      
      pair<size_t, size_t> &alignPoint = ntAlign[coIndex];
      if (sourceTarget == 0)
      {
        alignPoint.first = i;
      }
      else
      {
        alignPoint.second = i;
      }
    }
  }
  
  phrase = Join(" ", toks) + " [X]";
  
}

void ReformateHieroScore(string &scoreString)
{
  vector<string> toks;
  Tokenize(toks, scoreString, " ");

  for (size_t i = 0; i < toks.size(); ++i)
  {
    string &tok = toks[i];

    float score = Scan<float>(tok);
    score = exp(-score);
    tok = SPrint(score);
  }
  
  scoreString = Join(" ", toks);
}
  
string *ReformatHieroRule(const string &lineOrig)
{  
  vector<string> tokens;
  vector<float> scoreVector;
  
  TokenizeMultiCharSeparator(tokens, lineOrig, "|||" );

  string &sourcePhraseString = tokens[1]
              , &targetPhraseString = tokens[2]
              , &scoreString        = tokens[3];

  map<size_t, pair<size_t, size_t> > ntAlign;
  ReformatHieroRule(0, sourcePhraseString, ntAlign);
  ReformatHieroRule(1, targetPhraseString, ntAlign);
  ReformateHieroScore(scoreString);
  
  stringstream align;
  map<size_t, pair<size_t, size_t> >::const_iterator iterAlign;
  for (iterAlign = ntAlign.begin(); iterAlign != ntAlign.end(); ++iterAlign)
  {
    const pair<size_t, size_t> &alignPoint = iterAlign->second;
    align << alignPoint.first << "-" << alignPoint.second << " ";
  }
  
  stringstream ret;
  ret << sourcePhraseString << " ||| "
      << targetPhraseString << " ||| " 
      << scoreString << " ||| "
      << align.str();
  
  return new string(ret.str());
}
  
bool RuleTableLoaderStandard::Load(FormatType format
                                , const std::vector<FactorType> &input
                                , const std::vector<FactorType> &output
                                , std::istream &inStream
                                , const std::vector<float> &weight
                                , size_t /* tableLimit */
                                , const LMList &languageModels
                                , const WordPenaltyProducer* wpProducer
                                , RuleTableTrie &ruleTable)
{
  PrintUserTime("Start loading new format pt model");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();


  string lineOrig;
  size_t count = 0;

  while(getline(inStream, lineOrig)) {
    const string *line;
    if (format == HieroFormat) { // reformat line
      line = ReformatHieroRule(lineOrig);
    }
    else
    { // do nothing to format of line
      line = &lineOrig;
    }
    
    vector<string> tokens;
    vector<float> scoreVector;

    TokenizeMultiCharSeparator(tokens, *line , "|||" );

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
    CHECK(scoreVector.size() == numScoreComponents);

    // parse source & find pt node

    // constituent labels
    Word sourceLHS, targetLHS;

    // source
    Phrase sourcePhrase( 0);
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

    if (format == HieroFormat) { // reformat line
      delete line;
    }
    else
    { // do nothing
    }

  }

  // sort and prune each target phrase collection
  SortAndPrune(ruleTable);

  return true;
}

}
