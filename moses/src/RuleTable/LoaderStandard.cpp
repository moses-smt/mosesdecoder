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
#include "SpanLengthEstimator.h"
#include "DynSAInclude/types.h"

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
  PrintUserTime(string("Start loading text SCFG phrase table. ") + (format==MosesFormat?"Moses ":"Hiero ") + " format");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();


  string lineOrig;
  size_t count = 0;
  float ruleTotalCount = 1.0;
  float rcnt =1.0;
  vector<string> countStrings;

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
    vector<string> spanStrings;
    vector<string> spanStringsST;
    vector<string> spanStringSource;
    vector<string> spanStringTarget;

    TokenizeMultiCharSeparator(tokens, *line , "|||" );

    //Span Length branch : extended rule table to take span length into account, one more field
    if (tokens.size() < 4){
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


    ruleTotalCount = 1.0;
    rcnt =1.0;
    if (tokens.size() >= 5) {
      //MARIA
      //get rule counts tokens[4] -> count(t) assume that extract was run with  --NoFractionalCounting flag
      const std::string &ruleCount = tokens[4];
      countStrings.clear();

      TokenizeMultiCharSeparator(countStrings,ruleCount," ");
      if(countStrings.size()>=1){
        if(countStrings.size()==3)
          sscanf(countStrings[2].c_str(), "%e", &ruleTotalCount);
        else{
          sscanf(countStrings[0].c_str(), "%e", &rcnt);//&ruleTotalCount);   
          ruleTotalCount=floor(rcnt*scoreVector[0]+0.5);//ruleTotalCount*scoreVector[0]+0.5);     
        }
      //if(ruleTotalCount<=0.0) cerr<<"ERROR ruleTotalCount: "<<ruleTotalCount<< " " <<*line<<endl;
      }
    }

    //read from rule table
    std::vector<SpanLengthEstimator*> spanSourceEstimators, spanTargetEstimators;
    if (tokens.size() >= 6) {
      bool useGaussian = (StaticData::Instance().GetParam("gaussian-span-length-score").size() > 0);
      bool useISIFormat = (StaticData::Instance().GetParam("isi-format-for-span-length").size() > 0);
     
      const std::string &spanLength = tokens[5];

      //use ISI format for span_length information in rule table
      // rule_count | sum_NT1(len) | sum_NT1(len^2) || rule_count | sum_NT2(len) | sum_NT2(len^2) ...
      if (useISIFormat == true ){
	vector<string> spanStatisticsSource; 
	vector<string>::iterator itr_statistics;
	float count = 1.0;
	unsigned sum_len=0, sum_square_len=0;
	
	TokenizeMultiCharSeparator(spanStatisticsSource,spanLength,"||");
	//if(spanStatisticsSource.size()<2) continue; //it either has an entry for LHS and some NT or it has nothing
	//read all tuples but in SpanLengthFeature will only query the score given the LHS tuple
	for(itr_statistics = spanStatisticsSource.begin(); itr_statistics != spanStatisticsSource.end(); itr_statistics++){
	  vector<string> gaussParam;
	  TokenizeMultiCharSeparator(gaussParam,*itr_statistics,"|");
	  if(gaussParam.size()<3) continue;
	  sscanf(gaussParam[0].c_str(),"%f", &count);
	  sscanf(gaussParam[1].c_str(),"%u", &sum_len);
	  sscanf(gaussParam[2].c_str(),"%u", &sum_square_len);
	 
	  //first estimator added will correspond to the LHS statistics 
	  std::auto_ptr<SpanLengthEstimator> estimatorSource;
	  estimatorSource.reset(CreateGaussianSpanLengthEstimator());
	  estimatorSource->AddSpanScore_ISI(count,sum_len,sum_square_len);
	  estimatorSource->FinishedAdds(count);
	  spanSourceEstimators.push_back(estimatorSource.release());
       }
	
      }
      // use (len=score) format for span_length information in rule table
      else{

        //source and target side are separated by ||
        TokenizeMultiCharSeparator(spanStringsST,spanLength,"||");
      
        //we consider only source and target information
      
        if(spanStringsST.size()>=2){

          //Take scores from source
          string spanLengthSource = spanStringsST[0];
          //Take scores form target
          string spanLengthTarget = spanStringsST[1];

          TokenizeMultiCharSeparator(spanStringSource,spanLengthSource,"|");
          TokenizeMultiCharSeparator(spanStringTarget,spanLengthTarget,"|");

          //Check that number of non terminals is the same on both sides
          CHECK(spanStringSource.size() == spanStringTarget.size());
          vector<string>::iterator itr_source;
          vector<string>::iterator itr_target;
          for(itr_source = spanStringSource.begin(), itr_target = spanStringTarget.begin();
              itr_source != spanStringSource.end(), itr_target != spanStringTarget.end();
              itr_source++, itr_target++)
          {
            vector<string> spanTermSource;
            vector<string> spanTermTarget;
            vector<string> :: iterator itr_source_term;
            vector<string> :: iterator itr_target_term;
            Tokenize(spanTermSource,*itr_source);
            Tokenize(spanTermTarget,*itr_target);
            std::auto_ptr<SpanLengthEstimator> estimatorSource, estimatorTarget;
            estimatorSource.reset(useGaussian ? CreateGaussianSpanLengthEstimator() : CreateAsIsSpanLengthEstimator());
            estimatorTarget.reset(useGaussian ? CreateGaussianSpanLengthEstimator() : CreateAsIsSpanLengthEstimator());
            //get source scores
            iterate(spanTermSource,itr_source_term)
            {
              unsigned size;
              float proba;
              sscanf(itr_source_term->c_str(), "%u=%f", &size, &proba);
              estimatorSource->AddSpanScore(size, logf(proba));
            }
            //get target scores
            iterate(spanStringTarget,itr_target_term)
            {
              unsigned size;
              float proba;
              sscanf(itr_target_term->c_str(), "%u=%f", &size, &proba);
              estimatorTarget->AddSpanScore(size, logf(proba));
            }
            estimatorSource->FinishedAdds(ruleTotalCount);
            estimatorTarget->FinishedAdds(ruleTotalCount);
            spanSourceEstimators.push_back(estimatorSource.release());
            spanTargetEstimators.push_back(estimatorTarget.release());
          }
        }
      }
    }
        

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

    targetPhrase->SetScoreChart(ruleTable.GetFeature(), scoreVector, weight, languageModels, wpProducer);
    targetPhrase->SetSpanLengthEstimators(spanSourceEstimators, spanTargetEstimators);

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
