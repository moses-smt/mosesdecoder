// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "ContextSimilarityScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include <algorithm>
#include <iostream>

#define CONTEXT_SIZE 3
#define FACTOR_IDX 0

using namespace std;

namespace Moses
{

const FFState* ContextSimilarityScoreProducer::EmptyHypothesisState(const InputType &input) const
{
  return NULL;
}

FFState* ContextSimilarityScoreProducer::Evaluate(
  const Hypothesis& hypo,
  const FFState* prev_state,
  ScoreComponentCollection* out) const
{
  int start    = hypo.GetCurrSourceWordsRange().GetStartPos();
  int end      = hypo.GetCurrSourceWordsRange().GetEndPos() + 1; // STL-like
  int inputLen = hypo.GetInput().GetSize();
  set<string> currentContext;
  for (int i = start - CONTEXT_SIZE; i < start; i++) {
    string word = (i < 0) ? "<s>" : hypo.GetInput().GetWord(i).GetFactor(FACTOR_IDX)->GetString();
    currentContext.insert(word);
  }
  for (int i = end; i < end + CONTEXT_SIZE; i++) {
    string word = (i >= inputLen) ? "</s>" : hypo.GetInput().GetWord(i).GetFactor(FACTOR_IDX)->GetString();
    currentContext.insert(word);
  }
  out->PlusEquals(this, CosineSimilarity(currentContext, hypo.GetCurrTargetPhrase().GetContext()));
  return NULL;
}


float ContextSimilarityScoreProducer::CosineSimilarity(const set<string> &a, const set<string> &b) const
{
  if (a.empty() || b.empty()) return 0;
  vector<string> intersect;
  set_intersection(a.begin(), a.end(), b.begin(), b.end(), intersect.begin());
  return intersect.size() / (sqrt(a.size()) * sqrt(b.size()));
}

} // namespace Moses
