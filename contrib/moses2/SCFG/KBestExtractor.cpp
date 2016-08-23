/*
 * KBestExtractor.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "KBestExtractor.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "Stacks.h"
#include "Stack.h"
#include "Sentence.h"
#include "../System.h"
#include "../Scores.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{
NBestCandidate::NBestCandidate(const SCFG::Manager &mgr, const ArcList &varcList, size_t vind)
:arcList(&varcList)
,ind(vind)
{
	const SCFG::Hypothesis &hypo = GetHypo();

	// copy scores from best hypo
	MemPool &pool = mgr.GetPool();
	m_scores = new (pool.Allocate<Scores>())
				Scores(mgr.system,  pool, mgr.system.featureFunctions.GetNumScores(), hypo.GetScores());

	// children
	const ArcLists &arcLists = mgr.arcLists;
	//const SCFG::TargetPhraseImpl &tp = hypo.GetTargetPhrase();

	const Vector<const Hypothesis*> &prevHypos = hypo.GetPrevHypos();
	for (size_t i = 0; i < prevHypos.size(); ++i) {
		const SCFG::Hypothesis *prevHypo = prevHypos[i];
		const ArcList &childArc = arcLists.GetArcList(prevHypo);
		Child child(&childArc, 0);
		children.push_back(child);
	}
}

const SCFG::Hypothesis &NBestCandidate::GetHypo() const
{
	const HypothesisBase *hypoBase = (*arcList)[ind];
	const SCFG::Hypothesis &hypo = *static_cast<const SCFG::Hypothesis*>(hypoBase);
	return hypo;
}

void NBestCandidate::OutputToStream(
		const SCFG::Manager &mgr,
		std::stringstream &strm,
		const NBestColl &nbestColl) const
{
  const SCFG::TargetPhraseImpl &tp = GetHypo().GetTargetPhrase();

  for (size_t pos = 0; pos < tp.GetSize(); ++pos) {
	const SCFG::Word &word = tp[pos];
	//cerr << "word " << pos << "=" << word << endl;
	if (word.isNonTerminal) {
	  //cerr << "is nt" << endl;
	  // non-term. fill out with prev hypo
	  size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[pos];

	  UTIL_THROW_IF2(nonTermInd >= children.size(), "Out of bounds:" << nonTermInd << ">=" << children.size());

	  const Child &child = children[nonTermInd];
	  UTIL_THROW_IF2(child.first == NULL, "ArcList == NULL");

	  const NBestCandidates &nbests = nbestColl.GetNBestCandidates(*child.first);
	  const NBestCandidate &nbest = nbests[child.second];
	  nbest.OutputToStream(mgr, strm, nbestColl);
	}
	else {
	  //cerr << "not nt" << endl;
	  word.OutputToStream(strm);
	  strm << " ";
	}
  }

}

/////////////////////////////////////////////////////////////
void NBestColl::Add(const SCFG::Manager &mgr, const ArcList &arcList)
{
	NBestCandidate candidate(mgr, arcList, 0);
	m_candidates[&arcList].push_back(candidate);
}

const NBestCandidates &NBestColl::GetNBestCandidates(const ArcList &arcList) const
{
	Coll::const_iterator iter = m_candidates.find(&arcList);
	UTIL_THROW_IF2(iter == m_candidates.end(), "Can't find arclist");
	const NBestCandidates &ret = iter->second;
	return ret;
}

/////////////////////////////////////////////////////////////
KBestExtractor::KBestExtractor(const SCFG::Manager &mgr)
:m_mgr(mgr)
{
	const ArcLists &arcLists = mgr.arcLists;
	size_t inputSize = static_cast<const Sentence&>(mgr.GetInput()).GetSize();
	const Stacks &stacks = mgr.GetStacks();

	// set up n-best list for each hypo state
	for (size_t phraseSize = 1; phraseSize <= inputSize; ++phraseSize) {
		for (size_t startPos = 0; ; ++startPos) {
			size_t endPos = startPos + phraseSize - 1;
			if (endPos >= inputSize) {
				break;
			}

			const Stack &stack = stacks.GetStack(startPos, phraseSize);
			const Stack::Coll &allHypoColl = stack.GetColl();
			BOOST_FOREACH(const Stack::Coll::value_type &valPair, allHypoColl) {
				const HypothesisColl *hypoColl = valPair.second;
				const Hypotheses &sortedHypos = hypoColl->GetSortedAndPruneHypos(mgr, mgr.arcLists);
				BOOST_FOREACH(const HypothesisBase *hypoBase, sortedHypos) {
					const ArcList &arcList = arcLists.GetArcList(hypoBase);

					m_nbestColl.Add(mgr, arcList);
				}
			}
		}
	}
}

KBestExtractor::~KBestExtractor()
{
}

void KBestExtractor::OutputToStream(std::stringstream &strm)
{
	const Stack &lastStack = m_mgr.GetStacks().GetLastStack();
	UTIL_THROW_IF2(lastStack.GetColl().size() != 1, "Only suppose to be 1 hypo coll in last stack");
	UTIL_THROW_IF2(lastStack.GetColl().begin()->second == NULL, "NULL hypo collection");

	const Hypotheses &hypos = lastStack.GetColl().begin()->second->GetSortedAndPrunedHypos();
	UTIL_THROW_IF2(hypos.size() != 1, "Only suppose to be 1 hypo in collection");
	const HypothesisBase *hypo = hypos[0];

	const ArcLists &arcLists = m_mgr.arcLists;
	const ArcList &arcList = arcLists.GetArcList(hypo);
	const NBestCandidates &nbestVec = m_nbestColl.GetNBestCandidates(arcList);

	BOOST_FOREACH(const NBestCandidate &deriv, nbestVec) {
		strm << m_mgr.GetTranslationId() << " ||| ";
		//cerr << "1" << flush;
		deriv.OutputToStream(m_mgr, strm, m_nbestColl);
		//cerr << "2" << flush;
		strm << "||| ";
		deriv.GetScores().OutputBreakdown(strm, m_mgr.system);
		//cerr << "3" << flush;
		strm << "||| ";
		strm << deriv.GetScores().GetTotalScore();
		//cerr << "4" << flush;

		strm << endl;
	}
}

}
} /* namespace Moses2 */
