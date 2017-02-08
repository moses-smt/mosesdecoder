/*
 * NBest.cpp
 *
 *  Created on: 24 Aug 2016
 *      Author: hieu
 */
#include <sstream>
#include <boost/foreach.hpp>
#include "util/exception.hh"
#include "NBest.h"
#include "NBests.h"
#include "NBestColl.h"
#include "../Manager.h"
#include "../TargetPhraseImpl.h"
#include "../../System.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

NBest::NBest(
  const SCFG::Manager &mgr,
  const ArcList &varcList,
  size_t vind,
  NBestColl &nbestColl)
  :arcList(&varcList)
  ,arcInd(vind)
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
    NBests &childNBests = nbestColl.GetOrCreateNBests(mgr, childArc);
    Child child(&childNBests, 0);
    children.push_back(child);
  }

  stringstream strm;
  OutputToStream(mgr, strm);
  m_str = strm.str();
}

NBest::NBest(const SCFG::Manager &mgr,
             const NBest &orig,
             size_t childInd,
             NBestColl &nbestColl)
  :arcList(orig.arcList)
  ,arcInd(orig.arcInd)
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
  OutputToStream(mgr, strm);
  m_str = strm.str();
}

const SCFG::Hypothesis &NBest::GetHypo() const
{
  const HypothesisBase *hypoBase = (*arcList)[arcInd];
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
  NBestColl &nbestColl,
  Contenders &contenders) const
{
  if (arcInd + 1 < arcList->size()) {
    // to use next arclist, all children must be 1st. Not sure if this is correct
    bool ok = true;
    BOOST_FOREACH(const Child &child, children) {
      if (child.second) {
        ok = false;
        break;
      }
    }

    if (ok) {
      NBest *next = new NBest(mgr, *arcList, arcInd + 1, nbestColl);
      contenders.push(next);
    }
  }

  for (size_t childInd = 0; childInd < children.size(); ++childInd) {
    const Child &child = children[childInd];
    NBests &childNBests = *child.first;
    bool extended = childNBests.Extend(mgr, nbestColl, child.second + 1);
    if (extended) {
      //cerr << "HH1 " << childInd << endl;
      NBest *next = new NBest(mgr, *this, childInd, nbestColl);

      //cerr << "HH2 " << childInd << endl;
      contenders.push(next);
      //cerr << "HH3 " << childInd << endl;
    }
  }
}

void NBest::OutputToStream(
  const SCFG::Manager &mgr,
  std::stringstream &strm) const
{
  const SCFG::Hypothesis &hypo = GetHypo();
  //strm << &hypo << " ";

  const SCFG::TargetPhraseImpl &tp = hypo.GetTargetPhrase();

  for (size_t targetPos = 0; targetPos < tp.GetSize(); ++targetPos) {
    const SCFG::Word &word = tp[targetPos];
    //cerr << "word " << pos << "=" << word << endl;
    if (word.isNonTerminal) {
      //cerr << "is nt" << endl;
      // non-term. fill out with prev hypo
      size_t nonTermInd = tp.GetAlignNonTerm().GetNonTermIndexMap()[targetPos];

      UTIL_THROW_IF2(nonTermInd >= children.size(), "Out of bounds:" << nonTermInd << ">=" << children.size());

      const NBest &nbest = GetChild(nonTermInd);
      strm << nbest.GetString();
    } else {
      //cerr << "not nt" << endl;
      word.OutputToStream(hypo.GetManager(), targetPos, hypo, strm);

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
       << arcInd << "] ";
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

}
}
