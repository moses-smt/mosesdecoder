
#include <limits>
#include "Stack.h"
#include "Stacks.h"
#include "check.h"
#include "InputPath.h"
#include "TargetPhrase.h"
#include "TargetPhrases.h"
#include "WordsRange.h"
#include "Global.h"
#include "FF/TranslationModel/PhraseTable.h"
#include "FF/StatefulFeatureFunction.h"

using namespace std;

Stack::Stack()
{
  m_maxHypoStackSize = Global::Instance().stackSize;
  m_coll.reserve(m_maxHypoStackSize*2);
}

Stack::~Stack()
{
  // TODO Auto-generated destructor stub
}

bool Stack::AddPrune(Hypothesis *hypo)
{

  std::pair<iterator, bool> addRet = Add(hypo);
  if (addRet.second) {
    // added
    return true;
  }

  // recombine
  // equiv hypo exists, recombine with other hypo
  iterator &iterExisting = addRet.first;
  const Hypothesis *hypoExisting = *iterExisting;

  if (hypo->GetScores().GetWeightedScore() > hypoExisting->GetScores().GetWeightedScore()) {
    // incoming hypo is better than the one we have
    Remove(iterExisting);

    bool added = Add(hypo).second;
    assert(added);
    return false;
  } else {
    // already storing the best hypo. discard current hypo
    return false;
  }

}

std::pair<Stack::iterator, bool> Stack::Add(const Hypothesis *hypo)
{
  pair<iterator,bool> ret = m_coll.insert(hypo);
  if (ret.second) {
    // equiv hypo doesn't exists
    if (m_coll.size() > m_maxHypoStackSize * 2) {
      PruneToSize(m_maxHypoStackSize);
    }
  }

  return ret;
}

void Stack::PruneToSize()
{
  PruneToSize(m_maxHypoStackSize);
}

void Stack::PruneToSize(size_t newSize)
{
  if (m_coll.size() <= newSize ) {
    return; // not over limit
  }

  vector<const Hypothesis*> keep;
  SortHypotheses(newSize, keep);

  m_coll.clear();
  vector<const Hypothesis*>::const_iterator iter;
  for (iter = keep.begin(); iter != keep.end(); ++iter) {
    const Hypothesis *hypo = *iter;
    //cerr << "hypo" << hypo->Debug() << endl;
    std::pair<Stack::iterator, bool> ret = Add(hypo);
    CHECK(ret.second);
  }
}

void Stack::SortHypotheses(size_t newSize, vector<const Hypothesis*> &out)
{
  // sort hypotheses
  out.reserve(m_coll.size());
  std::copy(m_coll.begin(), m_coll.end(), std::inserter(out, out.end()));
  std::sort(out.begin(), out.end(), HypothesisScoreOrderer());

  // also keep those on boundary
  const Hypothesis &boundaryHypo = *out[newSize - 1];
  SCORE boundaryScore = boundaryHypo.GetScores().GetWeightedScore();

  for (size_t i = newSize; i < out.size(); ++i) {
    const Hypothesis *hypo = out[i];
    SCORE score = hypo->GetScores().GetWeightedScore();
    if (score < boundaryScore) {
      // score for this hypothesis is less than boundary score.
      // Discard this and all following hypos
      out.resize(i);
      break;
    }
  }
}

void Stack::Remove(Coll::iterator &iter)
{
  //const Hypothesis *hypo = *iter;
  size_t sizeBefore = m_coll.size();
  m_coll.erase(iter);
  assert(sizeBefore - m_coll.size() == 1);
}

void Stack::Search(const std::vector<InputPath*> &queue)
{
  for (iterator iter = begin(); iter != end(); ++iter) {
    const Hypothesis &hypo = **iter;
    Extend(hypo, queue);
  }
}

void Stack::Extend(const Hypothesis &hypo, const std::vector<InputPath*> &queue)
{
  //cerr << "extending " << hypo.Debug() << endl;
  const WordsBitmap &hypoCoverage = hypo.GetCoverage();

  for (size_t i = 0; i < queue.size(); ++i) {
    const InputPath &path = *queue[i];
    const WordsRange &range = path.GetRange();
    //cerr << range.Debug() << " " << hypoCoverage.Debug() << endl;
    if (!hypoCoverage.Overlap(range)) {
      Extend(hypo, path);
      //cerr << "EXTEND" << endl;
    } else {
      //cerr << "DONT EXTEND" << endl;
    }
  }
}

void Stack::Extend(const Hypothesis &hypo, const InputPath &path)
{
  const WordsRange &range = path.GetRange();
  const WordsRange &prevRange = hypo.GetRange();
  const WordsBitmap &coverage = hypo.GetCoverage();
  if (!coverage.WithinReorderingConstraint(prevRange, range)) {
    return;
  }

  size_t numPt = PhraseTable::GetColl().size();
  for (size_t i = 0; i < numPt; ++i) {
    const PhraseTableLookup &lookup = path.GetPtLookup(i);
    const TargetPhrases *tpColl = lookup.tpColl;
    if (tpColl) {
      Extend(hypo, *tpColl, range);
    }
  }
}

void Stack::Extend(const Hypothesis &hypo, const TargetPhrases &tpColl, const WordsRange &range)
{
  //cerr << "range=" << range.Debug() << " tpColl=" << tpColl.GetSize() << endl;
  WordsBitmap newCoverage(hypo.GetCoverage(), range);
  size_t wordsCovered = newCoverage.GetNumWordsCovered();

  TargetPhrases::const_iterator iter;
  for (iter = tpColl.begin(); iter != tpColl.end(); ++iter) {
    const TargetPhrase &tp = **iter;

    Hypothesis *newHypo = new Hypothesis(tp, hypo, range, newCoverage);
    
    StatefulFeatureFunction::Evaluate(*newHypo);

    bool added = m_stacks->Add(newHypo, wordsCovered);
    if (added) {
      //cerr << "added" << newHypo->Debug() << endl;
    } else {
      // discarded
      //delete newHypo;
    }
  }


}

std::string Stack::Debug() const
{
  stringstream strme;
  strme << GetSize();
  return strme.str();
}

const Hypothesis *Stack::GetHypothesis() const
{
  const Hypothesis *ret = NULL;
  SCORE bestScore = -std::numeric_limits<SCORE>::max();

  for (const_iterator iter = begin(); iter != end(); ++iter) {
    const Hypothesis *currHypo = *iter;
    SCORE currScore = currHypo->GetScores().GetWeightedScore();
    //cerr << currHypo->Debug() << endl;
    if (currScore > bestScore) {
      ret = currHypo;
      bestScore = currScore;
    }

  }
  return ret;
}
