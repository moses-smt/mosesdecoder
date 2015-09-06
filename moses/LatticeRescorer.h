#pragma once
#include <vector>
#include <map>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include "TypeDef.h"

namespace Moses
{
class HypothesisStack;
class Hypothesis;
class SameState;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// all hypos in SameStateAndPrev have same states AND same prevHypo
class SameStateAndPrev
{
public:
  boost::unordered_set<Hypothesis*> m_hypos;
  SameState *m_container;
  const Hypothesis *m_prevHypo; // key in container

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// all hypos in a node have same states
class SameState
{
  friend std::ostream& operator<<(std::ostream&, const SameState&);

  void OutputStackSize(const std::vector < HypothesisStack* > &stacks) const;

public:

  const Hypothesis *m_bestHypo; // key in graph

  typedef boost::unordered_map<const Hypothesis*, SameStateAndPrev*> HyposPerPrevHypo;
  HyposPerPrevHypo m_hypos;

  typedef boost::unordered_set<SameStateAndPrev*> FwdHypos;
  FwdHypos m_fwdHypos;

  boost::unordered_set<const Hypothesis*> m_newWinners;

  SameState(const Hypothesis *bestHypo);
  virtual ~SameState();

  inline bool operator==(const SameState &other) const {
    return m_bestHypo == other.m_bestHypo;
  }

  SameStateAndPrev &Add(Hypothesis *hypo);
  void AddEdge(SameStateAndPrev &edge);

  void Rescore(const std::vector < HypothesisStack* > &stacks, size_t pass, SameStateAndPrev *hypos);
  std::pair<AddStatus, const Hypothesis*> Rescore1Hypo(HypothesisStack &stack, Hypothesis *hypo, size_t pass);
  void DeleteFwdHypos();
  void DeleteHypos(SameStateAndPrev *hypos);

  void Multiply();
  void Multiply(SameStateAndPrev &hypos, const Hypothesis *prevHypo);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LatticeRescorerGraph
{
  friend std::ostream& operator<<(std::ostream&, const LatticeRescorerGraph&);

public:
  // 1st = bestHypo, 2nd = all hypos with same state as bestHypo, ie. bestHypo + losers
  typedef boost::unordered_map<const Hypothesis*, SameState*> Coll;
  //typedef std::set<SameState*, LatticeRescorerNodeComparer> Coll;
  Coll m_nodes;
  SameState *m_firstNode;

  void AddFirst(Hypothesis *bestHypo);
  void Add(Hypothesis *bestHypo);

  SameState &AddNode(const Hypothesis *bestHypo);

  void Rescore(const std::vector < HypothesisStack* > &stacks, size_t pass);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LatticeRescorer
{
public:

  void Rescore(const std::vector < HypothesisStack* > &stacks, size_t pass);

protected:
  std::vector < HypothesisStack* > *m_stacks;
  LatticeRescorerGraph m_graph;

  void OutputStackSize(const std::vector < HypothesisStack* > &stacks) const;


};

}

