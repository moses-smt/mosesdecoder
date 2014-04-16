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

namespace Moses
{

// Extract the k-best list from the search graph.
void ChartKBestExtractor::Extract(
    const std::vector<const ChartHypothesis*> &topHypos, std::size_t k,
    KBestVec &kBestList)
{
  typedef std::vector<const ChartHypothesis*> HypoVec;

  kBestList.clear();
  if (topHypos.empty()) {
    return;
  }

  // Create a new top-level ChartHypothesis that has the best hypothesis as its
  // predecessor.  This is the search hypergraph's target vertex.
  HypoVec::const_iterator iter = topHypos.begin();
  boost::scoped_ptr<ChartHypothesis> supremeHypo(
    new ChartHypothesis(**iter, *this));

  // Do the same for each alternative top-level hypothesis, but add the new
  // ChartHypothesis objects as arcs from supremeHypo, as if they had been
  // recombined.
  float prevScore = (*iter)->GetTotalScore();
  for (++iter; iter != topHypos.end(); ++iter) {
    // Check that the first item in topHypos really was the best.
    UTIL_THROW_IF2((*iter)->GetTotalScore() <= prevScore,
                   "top-level vertices are not correctly sorted");
    // Note: there's no need for a smart pointer here: supremeHypo will take
    // ownership of altHypo.
    ChartHypothesis *altHypo = new ChartHypothesis(**iter, *this);
    supremeHypo->AddArc(altHypo);
  }

  // Create the target vertex corresponding to supremeHypo then generate
  // it's k-best list.
  boost::shared_ptr<Vertex> top = FindOrCreateVertex(*supremeHypo);
  LazyKthBest(*top, k, k);

  // Copy the k-best list from the target vertex, but drop the top edge from
  // each derivation.
  kBestList.reserve(top->kBestList.size());
  for (KBestVec::const_iterator p = top->kBestList.begin();
       p != top->kBestList.end(); ++p) {
    const Derivation &d = **p;
    assert(d.edge->tail.size() == 1);  // d should have exactly one predecessor.
    assert(d.backPointers.size() == 1);
    std::size_t i = d.backPointers[0];
    boost::shared_ptr<Derivation> pred = d.edge.tail[0]->kBestList[i];
    kBestList.push_back(pred);
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
      const Derivation &subderivation =
        *d.edge.tail[nonTermInd]->kBestList[d.backPointers[nonTermInd]];
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

void ChartKBestExtractor::GetCandidates(Vertex &v, std::size_t k)
{
  // Create a derivation for v's best incoming edge.
  UnweightedHyperarc bestEdge = CreateEdge(v.hypothesis);
  boost::shared_ptr<Derivation> d(new Derivation(bestEdge));
  v.candidates.push(d);
  v.seen.insert(d);
  // Create derivations for the rest of v's incoming edges.
  const ChartArcList *arcList = v.hypothesis.GetArcList();
  if (arcList) {
    for (std::size_t i = 0; i < arcList->size(); ++i) {
      const ChartHypothesis &recombinedHypo = *(*arcList)[i];
      UnweightedHyperarc edge = CreateEdge(recombinedHypo);
      boost::shared_ptr<Derivation> d(new Derivation(edge));
      v.candidates.push(d);
      v.seen.insert(d);
    }
  }
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
  return sp;
}

void ChartKBestExtractor::LazyKthBest(Vertex &v, std::size_t k,
                                      std::size_t globalK)
{
  // If this is the first visit to vertex v then initialize the priority queue.
  if (v.visited == false) {
    GetCandidates(v, globalK);
    v.visited = true;
  }
  // Add derivations to the k-best list until it contains k or there are none
  // left to add.
  while (v.kBestList.size() < k) {
    if (!v.kBestList.empty()) {
      // Update the priority queue by adding the successors of the last
      // derivation (unless they've been seen before).
      const Derivation &d = *v.kBestList.back();
      LazyNext(v, d, globalK);
    }
    // Check if there are any derivations left in the queue.
    if (v.candidates.empty()) {
      break;
    }
    // Get the next best derivation and delete it from the queue.
    boost::shared_ptr<Derivation> d = v.candidates.top();
    v.candidates.pop();
    // Add it to the k-best list.
    v.kBestList.push_back(d);
  }
}

void ChartKBestExtractor::LazyNext(Vertex &v, const Derivation &d,
                                   std::size_t globalK)
{
  // Create the neighbours of Derivation d.
  for (std::size_t i = 0; i < d.backPointers.size(); ++i) {
    Vertex &predVertex = *d.edge.tail[i];
    // Ensure that predVertex's k-best list contains enough derivations.
    std::size_t k = d.backPointers[i] + 2;
    LazyKthBest(predVertex, k, globalK);
    if (predVertex.kBestList.size() < k) {
      // predVertex's derivations have been exhausted.
      continue;
    }
    // Create the neighbour.
    boost::shared_ptr<Derivation> next(new Derivation(d, i));
    // Check if it has been created before.
    std::pair<Vertex::DerivationSet::iterator, bool> p = v.seen.insert(next);
    if (p.second) {
      v.candidates.push(next);  // Haven't previously seen it.
    }
  }
}

// Construct a Derivation corresponding to a ChartHypothesis.
ChartKBestExtractor::Derivation::Derivation(const UnweightedHyperarc &e)
{
  edge = e;
  backPointers.resize(edge.tail.size(), 0);
  scoreBreakdown = edge.head->hypothesis.GetScoreBreakdown();
  score = edge.head->hypothesis.GetTotalScore();
}

// Construct a Derivation that neighbours an existing Derivation.
ChartKBestExtractor::Derivation::Derivation(const Derivation &d, std::size_t i)
{
  edge.head = d.edge.head;
  edge.tail = d.edge.tail;
  backPointers = d.backPointers;
  std::size_t j = ++backPointers[i];
  scoreBreakdown = d.scoreBreakdown;
  // Deduct the score of the old subderivation.
  const Derivation &oldSubderivation = *(edge.tail[i]->kBestList[j-1]);
  scoreBreakdown.MinusEquals(oldSubderivation.scoreBreakdown);
  // Add the score of the new subderivation.
  const Derivation &newSubderivation = *(edge.tail[i]->kBestList[j]);
  scoreBreakdown.PlusEquals(newSubderivation.scoreBreakdown);
  score = scoreBreakdown.GetWeightedScore();
}

}  // namespace Moses
