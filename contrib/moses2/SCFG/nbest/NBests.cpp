/*
 * NBests.cpp
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */

#include <boost/foreach.hpp>
#include "NBests.h"
#include "../Manager.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

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

void NBests::CreateDeviants(
		const SCFG::Manager &mgr,
		const ArcList &arcList,
		NBestColl &nbestColl)
{
	NBest *contender;

	// best
	contender = new NBest(mgr, arcList, 0, nbestColl);
	contenders.push(contender);

	size_t maxIter = mgr.system.options.nbest.nbest_size * mgr.system.options.nbest.factor;
	for (size_t i = 0; i < maxIter; ++i) {
		if (GetSize() >= mgr.system.options.nbest.nbest_size || contenders.empty()) {
			break;
		}

		contender = contenders.top();
		contenders.pop();

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

		contender->CreateDeviants(mgr, nbestColl, contenders);

		bool ok = false;
		if (mgr.system.options.nbest.only_distinct) {
			const string &tgtPhrase = contender->GetString();
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

		if (ok) {
			Add(contender);
			//cerr << best->GetScores().GetTotalScore() << " ";
			//cerr << best->Debug(mgr.system) << endl;
		}
		else {
			delete contender;
		}
	}
}


}
}

