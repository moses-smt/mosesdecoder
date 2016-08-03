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

  if (bestHypo) {
    TrellisPath *path = new TrellisPath(mgr, *bestHypo);
    m_coll.push_back(path);
  }
}

KBestExtractor::~KBestExtractor()
{
  // TODO Auto-generated destructor stub
}

void KBestExtractor::Output(std::stringstream &strm)
{
	BOOST_FOREACH(SCFG::TrellisPath *path, m_coll) {
		strm << m_mgr.GetTranslationId() << " ||| ";
		path->Output(strm);
		strm << endl;
	}
}

}
} /* namespace Moses2 */
