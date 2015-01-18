#include "KBestExtractor.h"

#include "moses/ScoreComponentCollection.h"
#include "moses/StaticData.h"

#include <boost/scoped_ptr.hpp>

#include <vector>

namespace Moses
{
namespace Syntax
{

// Extract the k-best list from the search graph.
void KBestExtractor::Extract(
  const std::vector<boost::shared_ptr<SVertex> > &topLevelVertices,
  std::size_t k, KBestVec &kBestList)
{
  kBestList.clear();
  if (topLevelVertices.empty()) {
    return;
  }

  // Create a new SVertex, supremeVertex, that has the best top-level SVertex as
  // its predecessor and has the same score.
  std::vector<boost::shared_ptr<SVertex> >::const_iterator p =
    topLevelVertices.begin();
  SVertex &bestTopLevelVertex = **p;
  boost::scoped_ptr<SVertex> supremeVertex(new SVertex());
  supremeVertex->pvertex = 0;
  supremeVertex->best = new SHyperedge();
  supremeVertex->best->head = supremeVertex.get();
  supremeVertex->best->tail.push_back(&bestTopLevelVertex);
  supremeVertex->best->score = bestTopLevelVertex.best->score;
  supremeVertex->best->scoreBreakdown = bestTopLevelVertex.best->scoreBreakdown;
  supremeVertex->best->translation = 0;

  // For each alternative top-level SVertex, add a new incoming hyperedge to
  // supremeVertex.
  for (++p; p != topLevelVertices.end(); ++p) {
    // Check that the first item in topLevelVertices really was the best.
    UTIL_THROW_IF2((*p)->best->score > bestTopLevelVertex.best->score,
                   "top-level SVertices are not correctly sorted");
    // Note: there's no need for a smart pointer here: supremeVertex will take
    // ownership of altEdge.
    SHyperedge *altEdge = new SHyperedge();
    altEdge->head = supremeVertex.get();
    altEdge->tail.push_back((*p).get());
    altEdge->score = (*p)->best->score;
    altEdge->scoreBreakdown = (*p)->best->scoreBreakdown;
    altEdge->translation = 0;
    supremeVertex->recombined.push_back(altEdge);
  }

  // Create the target vertex then lazily fill its k-best list.
  boost::shared_ptr<KVertex> targetVertex = FindOrCreateVertex(*supremeVertex);
  LazyKthBest(targetVertex, k, k);

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
Phrase KBestExtractor::GetOutputPhrase(const Derivation &d)
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

  Phrase ret(ARRAY_SIZE_INCR);

  const TargetPhrase &phrase = *(d.edge->shyperedge.translation);
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
      // FIXME
      UTIL_THROW2("placeholders are not currently supported by the S2T decoder");
      /*
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
      */
    }
  }

  return ret;
}

// Generate the target tree of the derivation d.
TreePointer KBestExtractor::GetOutputTree(const Derivation &d)
{
  const TargetPhrase &phrase = *(d.edge->shyperedge.translation);
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
  } else {
    UTIL_THROW2("Error: TreeStructureFeature active, but no internal tree structure found");
  }
}

// Look for the vertex corresponding to a given SVertex, creating
// a new one if necessary.
boost::shared_ptr<KBestExtractor::KVertex>
KBestExtractor::FindOrCreateVertex(const SVertex &v)
{
  // KVertex nodes should not be created for terminal nodes.
  assert(v.best);

  VertexMap::value_type element(&v, boost::shared_ptr<KVertex>());
  std::pair<VertexMap::iterator, bool> p = m_vertexMap.insert(element);
  boost::shared_ptr<KVertex> &sp = p.first->second;
  if (!p.second) {
    return sp;  // KVertex was already in m_vertexMap.
  }
  sp.reset(new KVertex(v));
  // Create the 1-best derivation and add it to the vertex's kBestList.
  boost::shared_ptr<KHyperedge> bestEdge(new KHyperedge(*(v.best)));
  bestEdge->head = sp;
  std::size_t kTailSize = 0;
  for (std::size_t i = 0; i < v.best->tail.size(); ++i) {
    const SVertex *pred = v.best->tail[i];
    if (pred->best) {
      ++kTailSize;
    }
  }
  bestEdge->tail.reserve(kTailSize);
  for (std::size_t i = 0; i < v.best->tail.size(); ++i) {
    const SVertex *pred = v.best->tail[i];
    if (pred->best) {
      bestEdge->tail.push_back(FindOrCreateVertex(*pred));
    }
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
void KBestExtractor::GetCandidates(boost::shared_ptr<KVertex> v, std::size_t k)
{
  // Create 1-best derivations for all of v's incoming edges except the best.
  // The 1-best derivation for that edge will already have been created.
  for (std::size_t i = 0; i < v->svertex.recombined.size(); ++i) {
    const SHyperedge &shyperedge = *(v->svertex.recombined[i]);
    boost::shared_ptr<KHyperedge> bestEdge(new KHyperedge(shyperedge));
    bestEdge->head = v;
    // Count the number of incoming vertices that are not terminals.
    std::size_t kTailSize = 0;
    for (std::size_t j = 0; j < shyperedge.tail.size(); ++j) {
      const SVertex *pred = shyperedge.tail[j];
      if (pred->best) {
        ++kTailSize;
      }
    }
    bestEdge->tail.reserve(kTailSize);
    for (std::size_t j = 0; j < shyperedge.tail.size(); ++j) {
      const SVertex *pred = shyperedge.tail[j];
      if (pred->best) {
        bestEdge->tail.push_back(FindOrCreateVertex(*pred));
      }
    }
    boost::shared_ptr<Derivation> derivation(new Derivation(bestEdge));
#ifndef NDEBUG
    std::pair<DerivationSet::iterator, bool> q =
#endif
      m_derivations.insert(derivation);
    assert(q.second);
    v->candidates.push(derivation);
  }
}

// Lazily fill v's k-best list.
void KBestExtractor::LazyKthBest(boost::shared_ptr<KVertex> v, std::size_t k,
                                 std::size_t globalK)
{
  // If this is the first visit to vertex v then initialize the priority queue.
  if (v->visited == false) {
    // The 1-best derivation should already be in v's k-best list.
    assert(v->kBestList.size() == 1);
    // Initialize v's priority queue.
    GetCandidates(v, globalK);
    v->visited = true;
  }
  // Add derivations to the k-best list until it contains k or there are none
  // left to add.
  while (v->kBestList.size() < k) {
    assert(!v->kBestList.empty());
    // Update the priority queue by adding the successors of the last
    // derivation (unless they've been seen before).
    boost::shared_ptr<Derivation> d(v->kBestList.back());
    LazyNext(*v, *d, globalK);
    // Check if there are any derivations left in the queue.
    if (v->candidates.empty()) {
      break;
    }
    // Get the next best derivation and delete it from the queue.
    boost::weak_ptr<Derivation> next = v->candidates.top();
    v->candidates.pop();
    // Add it to the k-best list.
    v->kBestList.push_back(next);
  }
}

// Create the neighbours of Derivation d and add them to v's candidate queue.
void KBestExtractor::LazyNext(KVertex &v, const Derivation &d,
                              std::size_t globalK)
{
  for (std::size_t i = 0; i < d.edge->tail.size(); ++i) {
    boost::shared_ptr<KVertex> pred = d.edge->tail[i];
    // Ensure that pred's k-best list contains enough derivations.
    std::size_t k = d.backPointers[i] + 2;
    LazyKthBest(pred, k, globalK);
    if (pred->kBestList.size() < k) {
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
KBestExtractor::Derivation::Derivation(const boost::shared_ptr<KHyperedge> &e)
{
  edge = e;
  std::size_t arity = edge->tail.size();
  backPointers.resize(arity, 0);
  subderivations.reserve(arity);
  for (std::size_t i = 0; i < arity; ++i) {
    const KVertex &pred = *(edge->tail[i]);
    assert(pred.kBestList.size() >= 1);
    boost::shared_ptr<Derivation> sub(pred.kBestList[0]);
    subderivations.push_back(sub);
  }
  score = edge->shyperedge.score;
  scoreBreakdown = edge->shyperedge.scoreBreakdown;
}

// Construct a Derivation that neighbours an existing Derivation.
KBestExtractor::Derivation::Derivation(const Derivation &d, std::size_t i)
{
  edge = d.edge;
  backPointers = d.backPointers;
  subderivations = d.subderivations;
  std::size_t j = ++backPointers[i];
  scoreBreakdown = d.scoreBreakdown;
  // Deduct the score of the old subderivation.
  scoreBreakdown.MinusEquals(subderivations[i]->scoreBreakdown);
  // Update the subderivation pointer.
  boost::shared_ptr<Derivation> newSub(edge->tail[i]->kBestList[j]);
  subderivations[i] = newSub;
  // Add the score of the new subderivation.
  scoreBreakdown.PlusEquals(subderivations[i]->scoreBreakdown);
  score = scoreBreakdown.GetWeightedScore();
}

}  // namespace Syntax
}  // namespace Moses
