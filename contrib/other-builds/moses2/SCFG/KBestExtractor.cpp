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

  SCFG::TrellisPath *currPath;
  if (bestHypo) {
	currPath = new SCFG::TrellisPath(mgr, *bestHypo);
    contenders.Add(currPath);
  }

  cerr << "nbest_size=" << mgr.system.options.nbest.nbest_size << endl;
  size_t bestInd = 1;
  while (bestInd < mgr.system.options.nbest.nbest_size && !contenders.empty()) {
	cerr << "bestInd=" << bestInd << endl;
	currPath->CreateDeviantPaths(contenders, mgr);

	currPath = contenders.Get();
    m_coll.push_back(currPath);

    ++bestInd;
  }
}

KBestExtractor::~KBestExtractor()
{
  // TODO Auto-generated destructor stub
}

void KBestExtractor::OutputToStream(std::stringstream &strm)
{
	cerr << "m_coll=" << m_coll.size() << endl;
	BOOST_FOREACH(SCFG::TrellisPath *path, m_coll) {
		strm << m_mgr.GetTranslationId() << " ||| ";
		path->OutputToStream(strm);
		path->GetScores().OutputBreakdown(strm, m_mgr.system);
		strm << "||| ";
		strm << path->GetScores().GetTotalScore();

		strm << endl;
	}
}

}
} /* namespace Moses2 */
