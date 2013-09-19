//Fabienne Braune
//Loader for l-MBOT rules
//e.g. in [MBOT] |||  [NP][NP] [NP][VP][PP]  [S] ||| [NP][NP]  [NP] || [NP][VP]  [VP] || [NP][PP]  [PP] ||| 0-0 || 1-0 || 1-0 |||
//scores are omitted above but in MBOT rules

//Commments on implementation
//Target side, scores and alignments have to be split at || and stored as discontiguous units but this is not done here but in the objects
//representing discontiguous target phrases (TargetPhraseMBOT) and alignments (AlignmentInfoMBOT)

#include "RuleTableLoaderMBOT.h"
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "PhraseDictionarySCFG.h"
#include "PhraseDictionaryMBOT.h"
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

//new : include target phrase
#include "TargetPhraseMBOT.h"

using namespace std;

namespace Moses
{
bool RuleTableLoaderMBOT::Load(const std::vector<FactorType> &input
                                   , const std::vector<FactorType> &output
                                   , std::istream &inStream
                                   , const std::vector<float> &weight
                                   , size_t tableLimit
                                   , const LMList &languageModels
                                   , const WordPenaltyProducer* wpProducer
                                   , PhraseDictionarySCFG &ruleTable)

{
  PrintUserTime("Start loading l-MBOT rules");

  const StaticData &staticData = StaticData::Instance();
  const std::string& factorDelimiter = staticData.GetFactorDelimiter();

  string lineOrig;
  size_t count = 0;

   while(getline(inStream, lineOrig)) {
    const string *line;
    line = &lineOrig;

    vector<string> tokens;
    vector<float> scoreVector;

    TokenizeMultiCharSeparator(tokens, *line , "|||" );

    if (tokens.size() != 5 && tokens.size() != 6) {
      stringstream strme;
      strme << "Syntax error at " << ruleTable.GetFilePath() << ":" << count;
      UserMessage::Add(strme.str());
      abort();
    }

    const string &sourcePhraseString = tokens[1]
               , &targetPhraseString = tokens[2]
               , &scoreString        = tokens[3]
      , &alignString        = tokens[4];


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

    // constituent labels
    Word sourceLHS;
    std::vector<Word> targetLHS;

    // source
    Phrase sourcePhrase(0);
    sourcePhrase.CreateFromStringNewFormat(Input, input, sourcePhraseString, factorDelimiter, sourceLHS);


    // create target phrase obj
    TargetPhraseMBOT *targetPhrase = new TargetPhraseMBOT(sourcePhrase);
    targetPhrase->CreateFromStringNewFormat(Output, output, targetPhraseString, factorDelimiter, targetLHS);

    //set source lhs in target phrase
    targetPhrase->SetSourceLHS(sourceLHS);

    targetPhrase->SetAlignmentInfo(alignString);

    targetPhrase->SetTargetLHSMBOT(targetLHS);

    // component score, for n-best output
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),TransformScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

    //new:inserted for testing
    //std::cout << "After transform"<< std::endl

    targetPhrase->SetScoreChart(ruleTable.GetFeature(), scoreVector, weight, languageModels, wpProducer);

    //std::cerr << "TARGET PHRASE MBOT : " << (*targetPhrase) << std::endl;

    TargetPhraseCollection &phraseColl = GetOrCreateTargetPhraseCollection(ruleTable, sourcePhrase, *targetPhrase, sourceLHS);
     phraseColl.Add(targetPhrase);
    //std::cout << "Phrase Coll "<< &phraseColl << std::endl;

    count++;
  }

  // sort and prune each target phrase collection
  SortAndPrune(ruleTable);


  return true;
}

}
