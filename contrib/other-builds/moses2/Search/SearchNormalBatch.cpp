/*
 * SearchNormalBatch.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include <algorithm>
#include <boost/foreach.hpp>
#include "SearchNormalBatch.h"
#include "Stack.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "../InputPaths.h"
#include "../TargetPhrases.h"
#include "../TargetPhrase.h"
#include "../System.h"
#include "../Sentence.h"
#include "../FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses2
{

SearchNormalBatch::SearchNormalBatch(Manager &mgr)
:Search(mgr)
,m_batchForEval(&mgr.system.GetBatchForEval())
{
	// TODO Auto-generated constructor stub

}

SearchNormalBatch::~SearchNormalBatch() {
	// TODO Auto-generated destructor stub
}

void SearchNormalBatch::Decode()
{

	// init stacks
	m_stacks.Init(m_mgr.GetInput().GetSize() + 1);

	const Bitmap &initBitmap = m_mgr.GetBitmaps().GetInitialBitmap();
	Hypothesis *initHypo = Hypothesis::Create(m_mgr);
	initHypo->Init(m_mgr.GetInputPaths().GetBlank(), m_mgr.GetInitPhrase(), initBitmap);
	initHypo->EmptyHypothesisState(m_mgr.GetInput());

	m_stacks.Add(initHypo, m_mgr.GetHypoRecycle());

	for (size_t stackInd = 0; stackInd < m_stacks.GetSize(); ++stackInd) {
		Decode(stackInd);

		cerr << m_stacks << endl;

		// delete stack to save mem
		if (stackInd < m_stacks.GetSize() - 1) {
			m_stacks.Delete(stackInd);
		}
		//cerr << m_stacks << endl;
	}
}

void SearchNormalBatch::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];

  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(m_mgr.system.stackSize, m_mgr.GetHypoRecycle());
  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
		Extend(*hypo);
  }

  m_batchForEval->Sort(HypothesisTargetPhraseOrderer());
  /*
  std::sort(m_batchForEval->GetData(),
		  m_batchForEval->GetData() + m_batchForEval->size(),
		  HypothesisTargetPhraseOrderer());

  cerr << "SORTED:" << endl;
  for (size_t i = 0; i < m_batchForEval->size(); ++i) {
	  Hypothesis *hypo = m_batchForEval->get(i);
	  cerr << hypo->GetTargetPhrase() << endl;
  }
  */

  // batch FF evaluation
  const std::vector<const StatefulFeatureFunction*> &sfffs = m_mgr.system.featureFunctions.GetStatefulFeatureFunctions();
  BOOST_FOREACH(const StatefulFeatureFunction *sfff, sfffs) {
	  sfff->EvaluateWhenApplied(*m_batchForEval);
  }

  AddHypos();
  m_batchForEval->Reset();

  //cerr << m_stacks << endl;

  // delete stack to save mem
  m_stacks.Delete(stackInd);
}

void SearchNormalBatch::Extend(const Hypothesis &hypo)
{
	const InputPaths &paths = m_mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath &path, paths) {
		Extend(hypo, path);
	}
}

void SearchNormalBatch::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Bitmap &hypoBitmap = hypo.GetBitmap();
	const Range &hypoRange = hypo.GetInputPath().range;
	const Range &pathRange = path.range;

	if (!CanExtend(hypoBitmap, hypoRange.GetEndPos(), pathRange)) {
		return;
	}

	//cerr << " YES" << endl;

    // extend this hypo
	const Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(hypoBitmap, pathRange);
    //SCORE estimatedScore = m_mgr.GetEstimatedScores().CalcFutureScore2(hypoBitmap, pathRange.GetStartPos(), pathRange.GetEndPos());
    SCORE estimatedScore = m_mgr.GetEstimatedScores().CalcEstimatedScore(newBitmap);

	const std::vector<const TargetPhrases*> &tpsAllPt = path.targetPhrases;
	for (size_t i = 0; i < tpsAllPt.size(); ++i) {
		const TargetPhrases *tps = tpsAllPt[i];
		if (tps) {
			Extend(hypo, *tps, path, newBitmap, estimatedScore);
		}
	}
}

void SearchNormalBatch::Extend(const Hypothesis &hypo,
		const TargetPhrases &tps,
		const InputPath &path,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  Extend(hypo, *tp, path, newBitmap, estimatedScore);
  }
}

void SearchNormalBatch::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const InputPath &path,
		const Bitmap &newBitmap,
		SCORE estimatedScore)
{
	Hypothesis *newHypo = Hypothesis::Create(m_mgr);
	newHypo->Init(hypo, path, tp, newBitmap, estimatedScore);
	newHypo->EvaluateWhenAppliedNonBatch();


	m_batchForEval->Add(newHypo);
}

void SearchNormalBatch::AddHypos()
{
  for (size_t i = 0; i < m_batchForEval->GetSize(); ++i) {
	Hypothesis *hypo = (*m_batchForEval)[i];
	m_stacks.Add(hypo, m_mgr.GetHypoRecycle());

	//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
	//stack.Prune(m_mgr.GetHypoRecycle(), m_mgr.system.stackSize, m_mgr.system.stackSize * 2);
  }
}

const Hypothesis *SearchNormalBatch::GetBestHypothesis() const
{
	const Stack &lastStack = m_stacks.Back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}

}

