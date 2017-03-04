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
NBests::NBests(const SCFG::Manager &mgr,
               const ArcList &arcList,
               NBestColl &nbestColl)
  :indIter(0)
{
  // best
  NBest *contender = new NBest(mgr, arcList, 0, nbestColl);
  contenders.push(contender);
  bool extended = Extend(mgr, nbestColl, 0);
  assert(extended);
}

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

bool NBests::Extend(const SCFG::Manager &mgr,
                    NBestColl &nbestColl,
                    size_t ind)
{
  if (ind < m_coll.size()) {
    // asking for 1 we've dont already
    return true;
  }

  assert(ind == m_coll.size());

  // checks
  if (ind >= mgr.system.options.nbest.nbest_size) {
    return false;
  }

  size_t maxIter = mgr.system.options.nbest.nbest_size * mgr.system.options.nbest.factor;

  // MAIN LOOP, create 1 new deriv.
  // The loop is for distinct nbest
  bool ok = false;
  while (!ok) {
    ++indIter;
    if (indIter > maxIter) {
      return false;
    }

    if (contenders.empty()) {
      return false;
    }

    NBest *contender = contenders.top();
    contenders.pop();

    contender->CreateDeviants(mgr, nbestColl, contenders);

    if (mgr.system.options.nbest.only_distinct) {
      const string &tgtPhrase = contender->GetString();
      //cerr << "tgtPhrase=" << tgtPhrase << endl;
      boost::hash<std::string> string_hash;
      size_t hash = string_hash(tgtPhrase);

      if (distinctHypos.insert(hash).second) {
        ok = true;
      }
    } else {
      ok = true;
    }

    if (ok) {
      Add(contender);
      //cerr << best->GetScores().GetTotalScore() << " ";
      //cerr << best->Debug(mgr.system) << endl;
      return true;
    } else {
      delete contender;
    }
  }

  return false;
}

}
}

