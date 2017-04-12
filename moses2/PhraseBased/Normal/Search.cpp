/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include "Search.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include "Stack.h"
#include "../Manager.h"
#include "../TrellisPath.h"
#include "../Sentence.h"
#include "../../TrellisPaths.h"
#include "../../InputPathsBase.h"
#include "../../Phrase.h"
#include "../../System.h"
#include "../../PhraseBased/TargetPhrases.h"

using namespace std;

namespace Moses2
{
namespace NSNormal
{

Search::Search(Manager &mgr)
  :Moses2::Search(mgr)
  , m_stacks(mgr)
{
  // TODO Auto-generated constructor stub

}

Search::~Search()
{
  // TODO Auto-generated destructor stub
}

void Search::Decode()
{
  // init stacks
  const Sentence &sentence = static_cast<const Sentence&>(mgr.GetInput());
  m_stacks.Init(mgr, sentence.GetSize() + 1);

  const Bitmap &initBitmap = mgr.GetBitmaps().GetInitialBitmap();
  Hypothesis *initHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
  initHypo->Init(mgr, mgr.GetInputPaths().GetBlank(), mgr.GetInitPhrase(),
                 initBitmap);
  initHypo->EmptyHypothesisState(mgr.GetInput());

  m_stacks.Add(initHypo, mgr.GetHypoRecycle(), mgr.arcLists);

  for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
    Decode(stackInd);
    //cerr << m_stacks << endl;

    // delete stack to save mem
    if (stackInd < m_stacks.GetSize() - 1) {
      m_stacks.Delete(stackInd);
    }
    //cerr << m_stacks.Debug(mgr.system) << endl;
  }
}

void Search::Decode(size_t stackInd)
{
  //cerr << "stackInd=" << stackInd << endl;
  Stack &stack = m_stacks[stackInd];
  if (&stack == &m_stacks.Back()) {
    // last stack. don't do anythin
    return;
  }

  const Hypotheses &hypos = stack.GetSortedAndPrunedHypos(mgr, mgr.arcLists);
  //cerr << "hypos=" << hypos.size() << endl;

  const InputPaths &paths = mgr.GetInputPaths();

  BOOST_FOREACH(const InputPathBase *path, paths) {
    BOOST_FOREACH(const HypothesisBase *hypo, hypos) {
      Extend(*static_cast<const Hypothesis*>(hypo), *static_cast<const InputPath*>(path));
    }
  }
}

void Search::Extend(const Hypothesis &hypo, const InputPath &path)
{
  const Bitmap &hypoBitmap = hypo.GetBitmap();
  const Range &hypoRange = hypo.GetInputPath().range;
  const Range &pathRange = path.range;

  if (!CanExtend(hypoBitmap, hypoRange.GetEndPos(), pathRange)) {
    return;
  }

  const ReorderingConstraint &reorderingConstraint = mgr.GetInput().GetReorderingConstraint();
  if (!reorderingConstraint.Check(hypoBitmap, pathRange.GetStartPos(), pathRange.GetEndPos())) {
    return;
  }

  // extend this hypo
  const Bitmap &newBitmap = mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
  //SCORE estimatedScore = mgr.GetEstimatedScores().CalcFutureScore2(bitmap, pathRange.GetStartPos(), pathRange.GetEndPos());
  SCORE estimatedScore = mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);

  size_t numPt = mgr.system.mappings.size();
  const TargetPhrases **tpsAllPt = path.targetPhrases;
  for (size_t i = 0; i < numPt; ++i) {
    const TargetPhrases *tps = tpsAllPt[i];
    if (tps) {
      Extend(hypo, *tps, path, newBitmap, estimatedScore);
    }
  }
}

void Search::Extend(const Hypothesis &hypo, const TargetPhrases &tps,
                    const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore)
{
  BOOST_FOREACH(const TargetPhraseImpl *tp, tps) {
    Extend(hypo, *tp, path, newBitmap, estimatedScore);
  }
}

void Search::Extend(const Hypothesis &hypo, const TargetPhraseImpl &tp,
                    const InputPath &path, const Bitmap &newBitmap, SCORE estimatedScore)
{
  Hypothesis *newHypo = Hypothesis::Create(mgr.GetSystemPool(), mgr);
  newHypo->Init(mgr, hypo, path, tp, newBitmap, estimatedScore);
  newHypo->EvaluateWhenApplied();

  m_stacks.Add(newHypo, mgr.GetHypoRecycle(), mgr.arcLists);

  //m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
  //stack.Prune(mgr.GetHypoRecycle(), mgr.system.stackSize, mgr.system.stackSize * 2);

}

const Hypothesis *Search::GetBestHypo() const
{
  const Stack &lastStack = m_stacks.Back();
  const Hypothesis *best = lastStack.GetBestHypo<Hypothesis>();
  return best;
}

void Search::AddInitialTrellisPaths(TrellisPaths<TrellisPath> &paths) const
{
  const Stack &lastStack = m_stacks.Back();
  const Hypotheses &hypos = lastStack.GetSortedAndPrunedHypos(mgr, mgr.arcLists);

  BOOST_FOREACH(const HypothesisBase *hypoBase, hypos) {
    const Hypothesis *hypo = static_cast<const Hypothesis*>(hypoBase);
    TrellisPath *path = new TrellisPath(hypo, mgr.arcLists);
    paths.Add(path);
  }
}

} // namespace
}

