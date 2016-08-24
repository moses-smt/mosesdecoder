/*
 * KBestExtractor.cpp
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <sstream>
#include "KBestExtractor.h"
#include "../Manager.h"
#include "../Hypothesis.h"
#include "../Stacks.h"
#include "../Stack.h"
#include "../Sentence.h"
#include "../../System.h"
#include "../../Scores.h"
#include "../../legacy/Util2.h"

using namespace std;

namespace Moses2
{
//bool g_debug = false;

namespace SCFG
{
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
