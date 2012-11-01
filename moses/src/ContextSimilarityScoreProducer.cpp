// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "ContextSimilarityScoreProducer.h"
#include "WordsRange.h"
#include "Util.h"
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
  map<string, int> currentContext;
  for (int i = start - CONTEXT_SIZE; i < start; i++) {
    string word = (i < 0) ? "<s>" : hypo.GetInput().GetWord(i).GetFactor(FACTOR_IDX)->GetString();
    currentContext.insert(make_pair(word, 1)); // disregards counts for now
  }
  for (int i = end; i < end + CONTEXT_SIZE; i++) {
    string word = (i >= inputLen) ? "</s>" : hypo.GetInput().GetWord(i).GetFactor(FACTOR_IDX)->GetString();
    currentContext.insert(make_pair(word, 1));
  }
  out->PlusEquals(this, CosineSimilarity(currentContext, hypo.GetCurrTargetPhrase().GetContext()));
  return NULL;
}


float ContextSimilarityScoreProducer::CosineSimilarity(const map<string, int> &a, const map<string, int> &b) const
{
  if (a.empty() || b.empty()) return 0;
  vector<string> intersect;
  set<string> a_set, b_set;
  map<string, int>::const_iterator it;
  for (it = a.begin(); it != a.end(); it++) a_set.insert(it->first);
  for (it = b.begin(); it != b.end(); it++) b_set.insert(it->first);

  // throw away the counts for now, just compute cosine similarity on sets (sparse binary vectors)
  set_intersection(a_set.begin(), a_set.end(), b_set.begin(), b_set.end(),
      inserter(intersect, intersect.begin()));
  float score = intersect.size() / (sqrt(a.size()) * sqrt(b.size()));
//  cerr << score << endl;
  return FloorScore(log(score));
}

} // namespace Moses
