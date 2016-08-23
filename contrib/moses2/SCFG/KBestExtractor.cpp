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
#include "TrellisPath.h"
#include "../TrellisPaths.h"
#include "../System.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

KBestExtractor::KBestExtractor(const SCFG::Manager &mgr)
:m_mgr(mgr)
{
	boost::unordered_set<size_t> distinctHypos;

	ArcLists &arcLists = mgr.arcLists;
	const Stack &lastStack = mgr.GetStacks().GetLastStack();
	const Hypothesis *bestHypo = lastStack.GetBestHypo();

	TrellisPaths<SCFG::TrellisPath> contenders;

	if (bestHypo) {
		SCFG::TrellisPath *path = new SCFG::TrellisPath(mgr, *bestHypo);
		contenders.Add(path);
	}

	//cerr << "nbest_size=" << mgr.system.options.nbest.nbest_size << endl;
	size_t maxIter = mgr.system.options.nbest.nbest_size * mgr.system.options.nbest.factor;
	size_t bestInd = 0;
	for (size_t i = 0; i < maxIter; ++i) {
		if (bestInd > mgr.system.options.nbest.nbest_size || contenders.empty()) {
			break;
		}

		SCFG::TrellisPath *path = contenders.Get();
		//cerr << "bestInd=" << bestInd << endl;
		//cerr << "currPath=" << path->Debug(m_mgr.system) << endl;

		bool ok = false;
		if (mgr.system.options.nbest.only_distinct) {
			string tgtPhrase = path->Output();
			//cerr << "tgtPhrase=" << tgtPhrase << endl;
			boost::hash<std::string> string_hash;
			size_t hash = string_hash(tgtPhrase);

			if (distinctHypos.insert(hash).second) {
				ok = true;
			}
		}
		else {
			ok = true;
		}

		path->CreateDeviantPaths(contenders, mgr);

		if (ok) {
			++bestInd;
			m_coll.push_back(path);
		}
		else {
			delete path;
		}
	}
	//cerr << "contenders=" << contenders.GetSize() << endl;
}

KBestExtractor::~KBestExtractor()
{
	//cerr << "m_coll=" << m_coll.size() << endl;
	BOOST_FOREACH(SCFG::TrellisPath *path, m_coll) {
		delete path;
	}
}

void KBestExtractor::OutputToStream(std::stringstream &strm)
{
	//cerr << "START OutputToStream m_coll=" << m_coll.size() << endl;
	BOOST_FOREACH(SCFG::TrellisPath *path, m_coll) {
		//cerr << path << " " << path->Debug(m_mgr.system) << endl;

		strm << m_mgr.GetTranslationId() << " ||| ";
		//cerr << "1" << flush;
		strm << path->Output();
		//cerr << "2" << flush;
		strm << " ||| ";
		path->GetScores().OutputBreakdown(strm, m_mgr.system);
		//cerr << "3" << flush;
		strm << "||| ";
		strm << path->GetScores().GetTotalScore();
		//cerr << "4" << flush;

		strm << endl;
	}
	//cerr << "FINISH OutputToStream " << endl;
}

}
} /* namespace Moses2 */
