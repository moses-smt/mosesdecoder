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

#pragma once

#include <cassert>
#include "ChartHypothesis.h"
#include "ScoreComponentCollection.h"
#include "FF/InternalTree.h"

#include <boost/unordered_set.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <queue>
#include <vector>

namespace Moses
{

// k-best list extractor that implements algorithm 3 from this paper:
//
//  Liang Huang and David Chiang
//  "Better k-best parsing"
//  In Proceedings of IWPT 2005
//
class ChartKBestExtractor
{
public:
  struct Vertex;

  struct UnweightedHyperarc {
    boost::shared_ptr<Vertex> head;
    std::vector<boost::shared_ptr<Vertex> > tail;
  };

  struct Derivation {
    Derivation(const UnweightedHyperarc &);
    Derivation(const Derivation &, std::size_t);

    UnweightedHyperarc edge;
    std::vector<std::size_t> backPointers;
    std::vector<boost::shared_ptr<Derivation> > subderivations;
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

  struct Vertex {
    typedef std::priority_queue<boost::weak_ptr<Derivation>,
            std::vector<boost::weak_ptr<Derivation> >,
            DerivationOrderer> DerivationQueue;

    Vertex(const ChartHypothesis &h) : hypothesis(h), visited(false) {}

    const ChartHypothesis &hypothesis;
    std::vector<boost::weak_ptr<Derivation> > kBestList;
    DerivationQueue candidates;
    bool visited;
  };

  typedef std::vector<boost::shared_ptr<Derivation> > KBestVec;

  // Extract the k-best list from the search hypergraph given the full, sorted
  // list of top-level vertices.
  void Extract(const std::vector<const ChartHypothesis*> &topHypos,
               std::size_t k, KBestVec &);

  static Phrase GetOutputPhrase(const Derivation &);
  static boost::shared_ptr<ScoreComponentCollection> GetOutputScoreBreakdown(const Derivation &);
  static TreePointer GetOutputTree(const Derivation &);

private:
  typedef boost::unordered_map<const ChartHypothesis *,
          boost::shared_ptr<Vertex> > VertexMap;

  struct DerivationHasher {
    std::size_t operator()(const boost::shared_ptr<Derivation> &d) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, d->edge.head);
      boost::hash_combine(seed, d->edge.tail);
      boost::hash_combine(seed, d->backPointers);
      return seed;
    }
  };

  struct DerivationEqualityPred {
    bool operator()(const boost::shared_ptr<Derivation> &d1,
                    const boost::shared_ptr<Derivation> &d2) const {
      return d1->edge.head == d2->edge.head &&
             d1->edge.tail == d2->edge.tail &&
             d1->backPointers == d2->backPointers;
    }
  };

  typedef boost::unordered_set<boost::shared_ptr<Derivation>, DerivationHasher,
          DerivationEqualityPred> DerivationSet;

  UnweightedHyperarc CreateEdge(const ChartHypothesis &);
  boost::shared_ptr<Vertex> FindOrCreateVertex(const ChartHypothesis &);
  void GetCandidates(Vertex &, std::size_t);
  void LazyKthBest(Vertex &, std::size_t, std::size_t);
  void LazyNext(Vertex &, const Derivation &, std::size_t);

  VertexMap m_vertexMap;
  DerivationSet m_derivations;
};

}  // namespace Moses
