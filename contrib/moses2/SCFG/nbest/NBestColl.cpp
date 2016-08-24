/*
 * NBestColl.cpp
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "util/exception.hh"
#include "NBestColl.h"
#include "NBests.h"
#include "../Manager.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

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

		contender = nbests.contenders.top();
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

		contender->CreateDeviants(mgr, *this, nbests.contenders);

		bool ok = false;
		if (mgr.system.options.nbest.only_distinct) {
			const string &tgtPhrase = contender->GetString();
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
			nbests.Add(contender);
			//cerr << best->GetScores().GetTotalScore() << " ";
			//cerr << best->Debug(mgr.system) << endl;
		}
		else {
			delete contender;
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


}
}

