/*
 * KBestExtractor.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include "KBestExtractor.h"
#include "Manager.h"
#include "Hypothesis.h"
#include "Stacks.h"
#include "Stack.h"
#include "Sentence.h"
#include "../System.h"
#include "../Scores.h"
#include "../legacy/Util2.h"

using namespace std;

namespace Moses2
{
//bool g_debug = false;

namespace SCFG
{
NBest::NBest(
		const SCFG::Manager &mgr,
		const NBestColl &nbestColl,
		const ArcList &varcList,
		size_t vind)
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
		const NBests &childNBests = nbestColl.GetNBests(childArc);
		Child child(&childNBests, 0);
		children.push_back(child);
	}

	stringstream strm;
	OutputToStream(mgr, strm, nbestColl);
	m_str = strm.str();
}

NBest::NBest(const SCFG::Manager &mgr,
		const NBestColl &nbestColl,
		const NBest &orig,
		size_t childInd)
:arcList(orig.arcList)
,ind(orig.ind)
,children(orig.children)
{
	Child &child = children[childInd];
	size_t &ind = child.second;
	++ind;
	UTIL_THROW_IF2(ind >= child.first->GetSize(),
			"out of bound:" << ind << ">=" << child.first->GetSize());

	// scores
	MemPool &pool = mgr.GetPool();
	m_scores = new (pool.Allocate<Scores>())
				Scores(mgr.system,
						pool,
						mgr.system.featureFunctions.GetNumScores(),
						orig.GetScores());

	const Scores &origScores = orig.GetChild(childInd).GetScores();
	const Scores &newScores = GetChild(childInd).GetScores();

	m_scores->MinusEquals(mgr.system, origScores);
	m_scores->PlusEquals(mgr.system, newScores);

	stringstream strm;
	OutputToStream(mgr, strm, nbestColl);
	m_str = strm.str();
}

const SCFG::Hypothesis &NBest::GetHypo() const
{
	const HypothesisBase *hypoBase = (*arcList)[ind];
	const SCFG::Hypothesis &hypo = *static_cast<const SCFG::Hypothesis*>(hypoBase);
	return hypo;
}

const NBest &NBest::GetChild(size_t ind) const
{
	const Child &child = children[ind];
	const NBests &nbests = *child.first;
	const NBest &nbest = nbests.Get(child.second);
	return nbest;
}


void NBest::CreateDeviants(
		const SCFG::Manager &mgr,
		const NBestColl &nbestColl,
		Contenders &contenders) const
{
	if (ind + 1 < arcList->size()) {
		// to use next arclist, all children must be 1st. Not sure if this is correct
		bool ok = true;
		BOOST_FOREACH(const Child &child, children) {
			if (child.second) {
				ok = false;
				break;
			}
		}

		if (ok) {
			NBest *next = new NBest(mgr, nbestColl, *arcList, ind + 1);
			contenders.push(next);
		}
	}

	for (size_t childInd = 0; childInd < children.size(); ++childInd) {
		const Child &child = children[childInd];
		if (child.second + 1 < child.first->GetSize()) {
			//cerr << "HH1 " << childInd << endl;
			NBest *next = new NBest(mgr, nbestColl, *this, childInd);

			//cerr << "HH2 " << childInd << endl;
			contenders.push(next);
			//cerr << "HH3 " << childInd << endl;
		}
	}
}

void NBest::OutputToStream(
		const SCFG::Manager &mgr,
		std::stringstream &strm,
		const NBestColl &nbestColl) const
{
  const SCFG::Hypothesis &hypo = GetHypo();
  //strm << &hypo << " ";

  const SCFG::TargetPhraseImpl &tp = hypo.GetTargetPhrase();

  for (size_t pos = 0; pos < tp.GetSize(); ++pos) {
	const SCFG::Word &word = tp[pos];
	//cerr << "word " << pos << "=" << word << endl;
	if (word.isNonTerminal) {
	  //cerr << "is nt" << endl;
	  // non-term. fill out with prev hypo
	  size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[pos];

	  UTIL_THROW_IF2(nonTermInd >= children.size(), "Out of bounds:" << nonTermInd << ">=" << children.size());

	  const NBest &nbest = GetChild(nonTermInd);
	  strm << nbest.GetString();
	}
	else {
	  //cerr << "not nt" << endl;
	  word.OutputToStream(strm);
	  strm << " ";
	}
  }
}

std::string NBest::Debug(const System &system) const
{
	stringstream strm;
	strm << GetScores().GetTotalScore() << " "
			<< arcList << "("
			<< arcList->size() << ")["
			<< ind << "] ";
	for (size_t i = 0; i < children.size(); ++i) {
		const Child &child = children[i];
		const NBest &childNBest = child.first->Get(child.second);

		strm << child.first << "("
				<< child.first->GetSize() << ")["
				<< child.second << "]";
		strm << childNBest.GetScores().GetTotalScore() << " ";
	}
	return strm.str();
}

/////////////////////////////////////////////////////////////
NBests::~NBests()
{
	BOOST_FOREACH(const NBest *nbest, m_coll) {
		delete nbest;
	}

	// delete bad contenders left in queue
	while (!contenders.empty()) {
		NBest *contender = contenders.top();
		contenders.pop();
		delete contender;
	}
}

/////////////////////////////////////////////////////////////
NBestColl::~NBestColl()
{
	BOOST_FOREACH(const Coll::value_type &valPair, m_candidates) {
		NBests *nbests = valPair.second;
		delete nbests;
	}
}

void NBestColl::Add(const SCFG::Manager &mgr, const ArcList &arcList)
{
	NBests &nbests = GetOrCreateNBests(arcList);
	//cerr << "nbests for " << &nbests << ":";

	NBest *contender;

	// best
	contender = new NBest(mgr, *this, arcList, 0);
	nbests.contenders.push(contender);

	size_t maxIter = mgr.system.options.nbest.nbest_size * mgr.system.options.nbest.factor;
	for (size_t i = 0; i < maxIter; ++i) {
		if (nbests.GetSize() >= mgr.system.options.nbest.nbest_size || nbests.contenders.empty()) {
			break;
		}

		NBest *best = nbests.contenders.top();
		nbests.contenders.pop();

		/*
		cerr << "contenders: " << best->GetScores().GetTotalScore() << " ";
		vector<NBest*> temp;
		while (!contenders.empty()) {
			NBest *t2 = contenders.top();
			contenders.pop();
			temp.push_back(t2);
			cerr << t2->GetScores().GetTotalScore() << " ";
		}
		cerr << endl;
		for (size_t t3 = 0; t3 < temp.size(); ++t3) {
			contenders.push(temp[t3]);
		}
		*/

		best->CreateDeviants(mgr, *this, nbests.contenders);

		bool ok = false;
		if (mgr.system.options.nbest.only_distinct) {
			const string &tgtPhrase = best->GetString();
			//cerr << "tgtPhrase=" << tgtPhrase << endl;
			boost::hash<std::string> string_hash;
			size_t hash = string_hash(tgtPhrase);

			if (nbests.distinctHypos.insert(hash).second) {
				ok = true;
			}
		}
		else {
			ok = true;
		}

		if (ok) {
			nbests.Add(best);
			//cerr << best->GetScores().GetTotalScore() << " ";
			//cerr << best->Debug(mgr.system) << endl;
		}
		else {
			delete best;
		}
	}
}

const NBests &NBestColl::GetNBests(const ArcList &arcList) const
{
	Coll::const_iterator iter = m_candidates.find(&arcList);
	UTIL_THROW_IF2(iter == m_candidates.end(), "Can't find arclist");
	const NBests &ret = *iter->second;
	return ret;
}

NBests &NBestColl::GetOrCreateNBests(const ArcList &arcList)
{
	NBests *ret;
	Coll::const_iterator iter = m_candidates.find(&arcList);
	if(iter == m_candidates.end()) {
		ret = new NBests();
		m_candidates[&arcList] = ret;
	}
	else {
		ret = iter->second;
	}
	return *ret;
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

			//cerr << "RANGE=" << startPos << " " << phraseSize << endl;
			//g_debug = startPos == 0 && phraseSize == 3;
			//g_debug = true;

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
	//cerr << "1" << flush;
	const Stack &lastStack = m_mgr.GetStacks().GetLastStack();
	UTIL_THROW_IF2(lastStack.GetColl().size() != 1, "Only suppose to be 1 hypo coll in last stack");
	UTIL_THROW_IF2(lastStack.GetColl().begin()->second == NULL, "NULL hypo collection");

	const Hypotheses &hypos = lastStack.GetColl().begin()->second->GetSortedAndPrunedHypos();
	UTIL_THROW_IF2(hypos.size() != 1, "Only suppose to be 1 hypo in collection");
	const HypothesisBase *hypo = hypos[0];

	const ArcLists &arcLists = m_mgr.arcLists;
	const ArcList &arcList = arcLists.GetArcList(hypo);
	const NBests &nbests = m_nbestColl.GetNBests(arcList);

	for (size_t i = 0; i < nbests.GetSize(); ++i) {
		const NBest &deriv = nbests.Get(i);
		strm << m_mgr.GetTranslationId() << " ||| ";
		//cerr << "1" << flush;
		strm << deriv.GetString();
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
