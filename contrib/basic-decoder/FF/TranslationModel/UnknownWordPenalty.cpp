/*
 * UnknownWordPenalty.cpp
 *
 *  Created on: 5 Oct 2013
 *      Author: hieu
 */
#include <string>
#include <iostream>
#include "UnknownWordPenalty.h"
#include "InputPath.h"
#include "TargetPhrase.h"
#include "TargetPhrases.h"
#include "WordsRange.h"
#include "Util.h"
#include "TypeDef.h"
#include "FF/FeatureFunction.h"

using namespace std;

UnknownWordPenalty::UnknownWordPenalty(const std::string line)
  :PhraseTable(line)
{
  ReadParameters();

}

UnknownWordPenalty::~UnknownWordPenalty()
{
  // TODO Auto-generated destructor stub
}

void UnknownWordPenalty::Lookup(const std::vector<InputPath*> &inputPathQueue)
{
  Scores *estimatedFutureScore = new Scores();

  for (size_t i = 0; i < inputPathQueue.size(); ++i) {
    InputPath &path = *inputPathQueue[i];
    PhraseTableLookup &ptLookup = path.GetPtLookup(m_ptId);

    const Phrase &source = path.GetPhrase();
    if (source.GetSize() == 1) {
      const Word &sourceWord = source.GetWord(0);
      string str = sourceWord.ToString();
      str = "UNK:" + str + ":UNK";

      Word targetWord;
      targetWord.CreateFromString(str);

      TargetPhrase *tp = new TargetPhrase(1);
      tp->Set(0, targetWord);
      tp->GetScores().Add(*this, LOWEST_SCORE);

      FeatureFunction::Evaluate(source, *tp, *estimatedFutureScore);

      TargetPhrases *tpColl = new TargetPhrases();
      m_targetPhrases.push_back(tpColl);
      tpColl->Add(tp);

      ptLookup.Set(tpColl, NULL);
    } else {
      ptLookup.Set(NULL, NULL);
    }
  }
}

void UnknownWordPenalty::CleanUpAfterSentenceProcessing(const Sentence &source)
{
  m_targetPhrases.clear();
}

