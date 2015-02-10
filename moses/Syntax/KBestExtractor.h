#pragma once

#include <cassert>

#include <queue>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/weak_ptr.hpp>

#include "moses/ScoreComponentCollection.h"
#include "moses/FF/InternalTree.h"

#include "SHyperedge.h"
#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

// k-best list extractor that implements algorithm 3 from this paper:
//
//  Liang Huang and David Chiang
//  "Better k-best parsing"
//  In Proceedings of IWPT 2005
//
class KBestExtractor
{
public:
  struct KVertex;

  struct KHyperedge {
    KHyperedge(const SHyperedge &e) : shyperedge(e) {}

    const SHyperedge &shyperedge;
    boost::shared_ptr<KVertex> head;
    std::vector<boost::shared_ptr<KVertex> > tail;
  };

  struct Derivation {
    Derivation(const boost::shared_ptr<KHyperedge> &);
    Derivation(const Derivation &, std::size_t);

    boost::shared_ptr<KHyperedge> edge;
    std::vector<std::size_t> backPointers;
    std::vector<boost::shared_ptr<Derivation> > subderivations;
    ScoreComponentCollection scoreBreakdown;
    float score;
  };

  struct DerivationOrderer {
    bool operator()(const boost::weak_ptr<Derivation> &d1,
                    const boost::weak_ptr<Derivation> &d2) const {
      boost::shared_ptr<Derivation> s1(d1);
      boost::shared_ptr<Derivation> s2(d2);
      return s1->score < s2->score;
    }
  };

  struct KVertex {
    typedef std::priority_queue<boost::weak_ptr<Derivation>,
            std::vector<boost::weak_ptr<Derivation> >,
            DerivationOrderer> DerivationQueue;

    KVertex(const SVertex &v) : svertex(v), visited(false) {}

    const SVertex &svertex;
    std::vector<boost::weak_ptr<Derivation> > kBestList;
    DerivationQueue candidates;
    bool visited;
  };

  typedef std::vector<boost::shared_ptr<Derivation> > KBestVec;

  // Extract the k-best list from the search hypergraph given the full, sorted
  // list of top-level SVertices.
  void Extract(const std::vector<boost::shared_ptr<SVertex> > &, std::size_t,
               KBestVec &);

  static Phrase GetOutputPhrase(const Derivation &);
  static TreePointer GetOutputTree(const Derivation &);

private:
  typedef boost::unordered_map<const SVertex *,
          boost::shared_ptr<KVertex> > VertexMap;

  struct DerivationHasher {
    std::size_t operator()(const boost::shared_ptr<Derivation> &d) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, &(d->edge->shyperedge));
      boost::hash_combine(seed, d->backPointers);
      return seed;
    }
  };

  struct DerivationEqualityPred {
    bool operator()(const boost::shared_ptr<Derivation> &d1,
                    const boost::shared_ptr<Derivation> &d2) const {
      return &(d1->edge->shyperedge) == &(d2->edge->shyperedge) &&
             d1->backPointers == d2->backPointers;
    }
  };

  typedef boost::unordered_set<boost::shared_ptr<Derivation>, DerivationHasher,
          DerivationEqualityPred> DerivationSet;

  boost::shared_ptr<KVertex> FindOrCreateVertex(const SVertex &);
  void GetCandidates(boost::shared_ptr<KVertex>, std::size_t);
  void LazyKthBest(boost::shared_ptr<KVertex>, std::size_t, std::size_t);
  void LazyNext(KVertex &, const Derivation &, std::size_t);

  VertexMap m_vertexMap;
  DerivationSet m_derivations;
};

}  // namespace Syntax
}  // namespace Moses
