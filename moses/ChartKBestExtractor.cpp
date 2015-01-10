/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2014 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "ChartKBestExtractor.h"

#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "StaticData.h"

#include <boost/scoped_ptr.hpp>

#include <vector>

using namespace std;

namespace Moses
{

// Extract the k-best list from the search graph.
void ChartKBestExtractor::Extract(
  const std::vector<const ChartHypothesis*> &topLevelHypos, std::size_t k,
  KBestVec &kBestList)
{
  kBestList.clear();
  if (topLevelHypos.empty()) {
    return;
  }

  // Create a new ChartHypothesis object, supremeHypo, that has the best
  // top-level hypothesis as its predecessor and has the same score.
  std::vector<const ChartHypothesis*>::const_iterator p = topLevelHypos.begin();
  const ChartHypothesis &bestTopLevelHypo = **p;
  boost::scoped_ptr<ChartHypothesis> supremeHypo(
    new ChartHypothesis(bestTopLevelHypo, *this));

  // Do the same for each alternative top-level hypothesis, but add the new
  // ChartHypothesis objects as arcs from supremeHypo, as if they had been
  // recombined.
  for (++p; p != topLevelHypos.end(); ++p) {
    // Check that the first item in topLevelHypos really was the best.
    UTIL_THROW_IF2((*p)->GetTotalScore() > bestTopLevelHypo.GetTotalScore(),
                   "top-level hypotheses are not correctly sorted");
    // Note: there's no need for a smart pointer here: supremeHypo will take
    // ownership of altHypo.
    ChartHypothesis *altHypo = new ChartHypothesis(**p, *this);
    supremeHypo->AddArc(altHypo);
  }

  // Create the target vertex then lazily fill its k-best list.
  boost::shared_ptr<Vertex> targetVertex = FindOrCreateVertex(*supremeHypo);
  LazyKthBest(*targetVertex, k, k);

  // Copy the k-best list from the target vertex, but drop the top edge from
  // each derivation.
  kBestList.reserve(targetVertex->kBestList.size());
  for (std::vector<boost::weak_ptr<Derivation> >::const_iterator
       q = targetVertex->kBestList.begin();
       q != targetVertex->kBestList.end(); ++q) {
    const boost::shared_ptr<Derivation> d(*q);
    assert(d);
    assert(d->subderivations.size() == 1);
    kBestList.push_back(d->subderivations[0]);
  }
}

// Generate the target-side yield of the derivation d.
Phrase ChartKBestExtractor::GetOutputPhrase(const Derivation &d)
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

  Phrase ret(ARRAY_SIZE_INCR);

  const ChartHypothesis &hypo = d.edge.head->hypothesis;
  const TargetPhrase &phrase = hypo.GetCurrTargetPhrase();
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    phrase.GetAlignNonTerm().GetNonTermIndexMap();
  for (std::size_t pos = 0; pos < phrase.GetSize(); ++pos) {
    const Word &word = phrase.GetWord(pos);
    if (word.IsNonTerminal()) {
      std::size_t nonTermInd = nonTermIndexMap[pos];
      const Derivation &subderivation = *d.subderivations[nonTermInd];
      Phrase subPhrase = GetOutputPhrase(subderivation);
      ret.Append(subPhrase);
    } else {
      ret.AddWord(word);
      if (placeholderFactor == NOT_FOUND) {
        continue;
      }
      std::set<std::size_t> sourcePosSet =
        phrase.GetAlignTerm().GetAlignmentsForTarget(pos);
      if (sourcePosSet.size() == 1) {
        const std::vector<const Word*> *ruleSourceFromInputPath =
          hypo.GetTranslationOption().GetSourceRuleFromInputPath();
        UTIL_THROW_IF2(ruleSourceFromInputPath == NULL,
                       "Source Words in of the rules hasn't been filled out");
        std::size_t sourcePos = *sourcePosSet.begin();
        const Word *sourceWord = ruleSourceFromInputPath->at(sourcePos);
        UTIL_THROW_IF2(sourceWord == NULL,
                       "Null source word at position " << sourcePos);
        const Factor *factor = sourceWord->GetFactor(placeholderFactor);
        if (factor) {
          ret.Back()[0] = factor;
        }
      }
    }
  }

  return ret;
}

// Generate the score breakdown of the derivation d.
boost::shared_ptr<ScoreComponentCollection> 
ChartKBestExtractor::GetOutputScoreBreakdown(const Derivation &d)
{
  const ChartHypothesis &hypo = d.edge.head->hypothesis;
  boost::shared_ptr<ScoreComponentCollection> scoreBreakdown(new ScoreComponentCollection());
  scoreBreakdown->PlusEquals(hypo.GetDeltaScoreBreakdown());
  const TargetPhrase &phrase = hypo.GetCurrTargetPhrase();
  const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
    phrase.GetAlignNonTerm().GetNonTermIndexMap();
  for (std::size_t pos = 0; pos < phrase.GetSize(); ++pos) {
    const Word &word = phrase.GetWord(pos);
    if (word.IsNonTerminal()) {
      std::size_t nonTermInd = nonTermIndexMap[pos];
      const Derivation &subderivation = *d.subderivations[nonTermInd];
      scoreBreakdown->PlusEquals(*GetOutputScoreBreakdown(subderivation));
    }
  }

  return scoreBreakdown;
}

// Generate the target tree of the derivation d.
TreePointer ChartKBestExtractor::GetOutputTree(const Derivation &d)
{
  const ChartHypothesis &hypo = d.edge.head->hypothesis;
  const TargetPhrase &phrase = hypo.GetCurrTargetPhrase();
  if (const PhraseProperty *property = phrase.GetProperty("Tree")) {
    const std::string *tree = property->GetValueString();
    TreePointer mytree (boost::make_shared<InternalTree>(*tree));

    //get subtrees (in target order)
    std::vector<TreePointer> previous_trees;
    for (size_t pos = 0; pos < phrase.GetSize(); ++pos) {
      const Word &word = phrase.GetWord(pos);
      if (word.IsNonTerminal()) {
        size_t nonTermInd = phrase.GetAlignNonTerm().GetNonTermIndexMap()[pos];
        const Derivation &subderivation = *d.subderivations[nonTermInd];
        const TreePointer prev_tree = GetOutputTree(subderivation);
        previous_trees.push_back(prev_tree);
      }
    }

    mytree->Combine(previous_trees);
    return mytree;
  }
  else {
    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
  }
}

// Create an unweighted hyperarc corresponding to the given ChartHypothesis.
ChartKBestExtractor::UnweightedHyperarc ChartKBestExtractor::CreateEdge(
  const ChartHypothesis &h)
{
  UnweightedHyperarc edge;
  edge.head = FindOrCreateVertex(h);
  const std::vector<const ChartHypothesis*> &prevHypos = h.GetPrevHypos();
  edge.tail.resize(prevHypos.size());
  for (std::size_t i = 0; i < prevHypos.size(); ++i) {
    const ChartHypothesis *prevHypo = prevHypos[i];
    edge.tail[i] = FindOrCreateVertex(*prevHypo);
  }
  return edge;
}

// Look for the vertex corresponding to a given ChartHypothesis, creating
// a new one if necessary.
boost::shared_ptr<ChartKBestExtractor::Vertex>
ChartKBestExtractor::FindOrCreateVertex(const ChartHypothesis &h)
{
  VertexMap::value_type element(&h, boost::shared_ptr<Vertex>());
  std::pair<VertexMap::iterator, bool> p = m_vertexMap.insert(element);
  boost::shared_ptr<Vertex> &sp = p.first->second;
  if (!p.second) {
    return sp;  // Vertex was already in m_vertexMap.
  }
  sp.reset(new Vertex(h));
  // Create the 1-best derivation and add it to the vertex's kBestList.
  UnweightedHyperarc bestEdge;
  bestEdge.head = sp;
  const std::vector<const ChartHypothesis*> &prevHypos = h.GetPrevHypos();
  bestEdge.tail.resize(prevHypos.size());
  for (std::size_t i = 0; i < prevHypos.size(); ++i) {
    const ChartHypothesis *prevHypo = prevHypos[i];
    bestEdge.tail[i] = FindOrCreateVertex(*prevHypo);
  }
  boost::shared_ptr<Derivation> bestDerivation(new Derivation(bestEdge));
#ifndef NDEBUG
  std::pair<DerivationSet::iterator, bool> q =
#endif
    m_derivations.insert(bestDerivation);
  assert(q.second);
  sp->kBestList.push_back(bestDerivation);
  return sp;
}

// Create the 1-best derivation for each edge in BS(v) (except the best one)
// and add it to v's candidate queue.
void ChartKBestExtractor::GetCandidates(Vertex &v, std::size_t k)
{
  // Create derivations for all of v's incoming edges except the best.  This
  // means everything in v.hypothesis.GetArcList() and not the edge defined
  // by v.hypothesis itself.  The 1-best derivation for that edge will already
  // have been created.
  const ChartArcList *arcList = v.hypothesis.GetArcList();
  if (arcList) {
    for (std::size_t i = 0; i < arcList->size(); ++i) {
      const ChartHypothesis &recombinedHypo = *(*arcList)[i];
      boost::shared_ptr<Vertex> w = FindOrCreateVertex(recombinedHypo);
      assert(w->kBestList.size() == 1);
      v.candidates.push(w->kBestList[0]);
    }
  }
}

// Lazily fill v's k-best list.
void ChartKBestExtractor::LazyKthBest(Vertex &v, std::size_t k,
                                      std::size_t globalK)
{
  // If this is the first visit to vertex v then initialize the priority queue.
  if (v.visited == false) {
    // The 1-best derivation should already be in v's k-best list.
    assert(v.kBestList.size() == 1);
    // Initialize v's priority queue.
    GetCandidates(v, globalK);
    v.visited = true;
  }
  // Add derivations to the k-best list until it contains k or there are none
  // left to add.
  while (v.kBestList.size() < k) {
    assert(!v.kBestList.empty());
    // Update the priority queue by adding the successors of the last
    // derivation (unless they've been seen before).
    boost::shared_ptr<Derivation> d(v.kBestList.back());
    LazyNext(v, *d, globalK);
    // Check if there are any derivations left in the queue.
    if (v.candidates.empty()) {
      break;
    }
    // Get the next best derivation and delete it from the queue.
    boost::weak_ptr<Derivation> next = v.candidates.top();
    v.candidates.pop();
    // Add it to the k-best list.
    v.kBestList.push_back(next);
  }
}

// Create the neighbours of Derivation d and add them to v's candidate queue.
void ChartKBestExtractor::LazyNext(Vertex &v, const Derivation &d,
                                   std::size_t globalK)
{
  for (std::size_t i = 0; i < d.edge.tail.size(); ++i) {
    Vertex &pred = *d.edge.tail[i];
    // Ensure that pred's k-best list contains enough derivations.
    std::size_t k = d.backPointers[i] + 2;
    LazyKthBest(pred, k, globalK);
    if (pred.kBestList.size() < k) {
      // pred's derivations have been exhausted.
      continue;
    }
    // Create the neighbour.
    boost::shared_ptr<Derivation> next(new Derivation(d, i));
    // Check if it has been created before.
    std::pair<DerivationSet::iterator, bool> p = m_derivations.insert(next);
    if (p.second) {
      v.candidates.push(next);  // Haven't previously seen it.
    }
  }
}

// Construct the 1-best Derivation that ends at edge e.
ChartKBestExtractor::Derivation::Derivation(const UnweightedHyperarc &e)
{
  edge = e;
  std::size_t arity = edge.tail.size();
  backPointers.resize(arity, 0);
  subderivations.reserve(arity);
  for (std::size_t i = 0; i < arity; ++i) {
    const Vertex &pred = *edge.tail[i];
    assert(pred.kBestList.size() >= 1);
    boost::shared_ptr<Derivation> sub(pred.kBestList[0]);
    subderivations.push_back(sub);
  }
  score = edge.head->hypothesis.GetTotalScore();
}

// Construct a Derivation that neighbours an existing Derivation.
ChartKBestExtractor::Derivation::Derivation(const Derivation &d, std::size_t i)
{
  edge.head = d.edge.head;
  edge.tail = d.edge.tail;
  backPointers = d.backPointers;
  subderivations = d.subderivations;
  std::size_t j = ++backPointers[i];
  score = d.score;
  // Deduct the score of the old subderivation.
  score -= subderivations[i]->score;
  // Update the subderivation pointer.
  boost::shared_ptr<Derivation> newSub(edge.tail[i]->kBestList[j]);
  subderivations[i] = newSub;
  // Add the score of the new subderivation.
  score += subderivations[i]->score;
}

}  // namespace Moses
