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
  ArcLists &arcLists = mgr.arcLists;
  const Stack &lastStack = mgr.GetStacks().GetLastStack();
  const Hypothesis *bestHypo = lastStack.GetBestHypo(mgr, arcLists);

  TrellisPaths<SCFG::TrellisPath> contenders;

  if (bestHypo) {
    SCFG::TrellisPath *path = new SCFG::TrellisPath(mgr, *bestHypo);
    contenders.Add(path);
  }

  cerr << "nbest_size=" << mgr.system.options.nbest.nbest_size << endl;
  size_t bestInd = 0;
  while (bestInd < mgr.system.options.nbest.nbest_size && !contenders.empty()) {
    SCFG::TrellisPath *path = contenders.Get();
    cerr << "bestInd=" << bestInd << endl;
    cerr << "currPath=" << path->Debug(m_mgr.system) << endl;

    path->CreateDeviantPaths(contenders, mgr);

    m_coll.push_back(path);

    ++bestInd;
  }
}

KBestExtractor::~KBestExtractor()
{
  // TODO Auto-generated destructor stub
}

void KBestExtractor::OutputToStream(std::stringstream &strm)
{
	//cerr << "m_coll=" << m_coll.size() << endl;
	BOOST_FOREACH(SCFG::TrellisPath *path, m_coll) {
	  cerr << path << " " << path->Debug(m_mgr.system) << endl;

		strm << m_mgr.GetTranslationId() << " ||| ";
		strm << path->Output();
    strm << " ||| ";
		path->GetScores().OutputBreakdown(strm, m_mgr.system);
		strm << "||| ";
		strm << path->GetScores().GetTotalScore();

		strm << endl;
	}
}

}
} /* namespace Moses2 */
