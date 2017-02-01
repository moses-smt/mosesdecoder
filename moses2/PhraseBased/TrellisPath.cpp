/*
 * TrellisPath.cpp
 *
 *  Created on: 16 Mar 2016
 *      Author: hieu
 */
#include <cassert>
#include <sstream>
#include "TrellisPath.h"
#include "Hypothesis.h"
#include "InputPath.h"
#include "../TrellisPaths.h"
#include "../System.h"
#include "../SubPhrase.h"

using namespace std;

namespace Moses2
{

std::string TrellisNode::Debug(const System &system) const
{
  stringstream out;
  out << "arcList=" << arcList->size() << " " << ind;
  return out.str();
}

/////////////////////////////////////////////////////////////////////////////////
TrellisPath::TrellisPath(const Hypothesis *hypo, const ArcLists &arcLists) :
  prevEdgeChanged(-1)
{
  AddNodes(hypo, arcLists);
  m_scores = &hypo->GetScores();
}

TrellisPath::TrellisPath(const TrellisPath &origPath, size_t edgeIndex,
                         const TrellisNode &newNode, const ArcLists &arcLists, MemPool &pool,
                         const System &system) :
  prevEdgeChanged(edgeIndex)
{
  nodes.reserve(origPath.nodes.size());
  for (size_t currEdge = 0; currEdge < edgeIndex; currEdge++) {
    // copy path from parent
    nodes.push_back(origPath.nodes[currEdge]);
  }

  // 1 deviation
  nodes.push_back(newNode);

  // rest of path comes from following best path backwards
  const Hypothesis *arc = static_cast<const Hypothesis*>(newNode.GetHypo());

  const Hypothesis *prevHypo = arc->GetPrevHypo();
  while (prevHypo != NULL) {
    const ArcList &arcList = arcLists.GetArcList(prevHypo);
    TrellisNode node(arcList, 0);
    nodes.push_back(node);

    prevHypo = prevHypo->GetPrevHypo();
  }

  const TrellisNode &origNode = origPath.nodes[edgeIndex];
  const HypothesisBase *origHypo = origNode.GetHypo();
  const HypothesisBase *newHypo = newNode.GetHypo();

  CalcScores(origPath.GetScores(), origHypo->GetScores(), newHypo->GetScores(),
             pool, system);
}

TrellisPath::~TrellisPath()
{
  // TODO Auto-generated destructor stub
}

SCORE TrellisPath::GetFutureScore() const
{
  return m_scores->GetTotalScore();
}

std::string TrellisPath::Debug(const System &system) const
{
  stringstream out;

  out << OutputTargetPhrase(system);
  out << "||| ";

  out << GetScores().Debug(system);
  out << "||| ";

  out << GetScores().GetTotalScore();

  return out.str();
}

void TrellisPath::OutputToStream(std::ostream &out, const System &system) const
{
  out << OutputTargetPhrase(system);
  out << "||| ";

  GetScores().OutputBreakdown(out, system);
  out << "||| ";

  out << GetScores().GetTotalScore();
}

std::string TrellisPath::OutputTargetPhrase(const System &system) const
{
  std::stringstream out;
  for (int i = nodes.size() - 2; i >= 0; --i) {
    const TrellisNode &node = nodes[i];

    const Hypothesis *hypo = static_cast<const Hypothesis*>(node.GetHypo());
    const TargetPhrase<Moses2::Word> &tp = hypo->GetTargetPhrase();

    const InputPath &path = static_cast<const InputPath&>(hypo->GetInputPath());
    const SubPhrase<Moses2::Word> &subPhrase = path.subPhrase;

    tp.OutputToStream(system, subPhrase, out);
  }
  return out.str();
}

void TrellisPath::CreateDeviantPaths(TrellisPaths<TrellisPath> &paths,
                                     const ArcLists &arcLists, MemPool &pool, const System &system) const
{
  const size_t sizePath = nodes.size();

  //cerr << "prevEdgeChanged=" << prevEdgeChanged << endl;
  for (size_t currEdge = prevEdgeChanged + 1; currEdge < sizePath; currEdge++) {
    TrellisNode newNode = nodes[currEdge];
    assert(newNode.ind == 0);
    const ArcList &arcList = *newNode.arcList;

    //cerr << "arcList=" << arcList.size() << endl;
    for (size_t i = 1; i < arcList.size(); ++i) {
      //cerr << "i=" << i << endl;
      newNode.ind = i;

      TrellisPath *deviantPath = new TrellisPath(*this, currEdge, newNode,
          arcLists, pool, system);
      //cerr << "deviantPath=" << deviantPath << endl;
      paths.Add(deviantPath);
    }
  }
}

void TrellisPath::CalcScores(const Scores &origScores,
                             const Scores &origHypoScores, const Scores &newHypoScores, MemPool &pool,
                             const System &system)
{
  Scores *scores = new (pool.Allocate<Scores>()) Scores(system, pool,
      system.featureFunctions.GetNumScores(), origScores);
  scores->PlusEquals(system, newHypoScores);
  scores->MinusEquals(system, origHypoScores);

  m_scores = scores;
}

void TrellisPath::AddNodes(const Hypothesis *hypo, const ArcLists &arcLists)
{
  if (hypo) {
    // add this hypo
    //cerr << "hypo=" << hypo << " " << flush;
    //cerr << *hypo << endl;
    const ArcList &list = arcLists.GetArcList(hypo);
    TrellisNode node(list, 0);
    nodes.push_back(node);

    // add prev hypos
    const Hypothesis *prev = hypo->GetPrevHypo();
    AddNodes(prev, arcLists);
  }
}

} /* namespace Moses2 */
