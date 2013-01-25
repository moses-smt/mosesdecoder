// $Id: LeftContextScoreProducer.cpp,v 1.1 2012/10/07 13:43:03 braunefe Exp $

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "LeftContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{

LeftContextScoreProducer::LeftContextScoreProducer(ScoreIndexManager &scoreIndexManager, float weight)
{
  scoreIndexManager.AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);
  srcFactors.push_back(0);
  tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

const FFState* LeftContextScoreProducer::EmptyHypothesisState(const InputType &input) const
{
  return NULL; // don't need previous states
}

void LeftContextScoreProducer::LoadScores(const string &ttableFile)
{
  ifstream ttableIn(ttableFile.c_str());

  string line;
  while (getline(ttableIn, line)) {
    vector<string> segments;
    TokenizeMultiCharSeparator(segments, line, " ||| ");

    // get the third score: p(e|f)
    vector<string> scores;
    Tokenize(scores, segments[2], " ");
    float p_e_f = Scan<float>(scores[2]);

    pair<string, string> phrasePair = make_pair(segments[0], segments[1]);
    modelScores.insert(make_pair(phrasePair, p_e_f));
  }
  ttableIn.close();
}

FFState* LeftContextScoreProducer::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* out) const
{

  TTable::const_iterator it;
  float score = 0;

  // get the word to the left
  size_t startPos = hypo.GetCurrSourceWordsRange().GetStartPos();
  string wordToTheLeft = "{";
  if (startPos > 0) {
    wordToTheLeft.append(hypo.GetInput().GetWord(startPos - 1).GetString(srcFactors, false));
  }
  wordToTheLeft.append("} ");

  // get the source and target phrase
  string srcPhrase = hypo.GetSourcePhraseStringRep(srcFactors);
  string tgtPhrase = hypo.GetTargetPhraseStringRep(tgtFactors);

  // prepend the left context
  srcPhrase = wordToTheLeft + srcPhrase;

  if ((it = modelScores.find(make_pair(srcPhrase, tgtPhrase))) != modelScores.end()) {
    score = log(it->second);
  } 

  out->PlusEquals(this, score); 
  
  return NULL;
}

size_t LeftContextScoreProducer::GetNumScoreComponents() const
{
  return 1; // let's return just P(e|f) for now
}

std::string LeftContextScoreProducer::GetScoreProducerDescription(unsigned) const
{
  return "LeftContext";
}

std::string LeftContextScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "lc";
}

size_t LeftContextScoreProducer::GetNumInputScores() const
{
  return 0;
}

} // namespace Moses
