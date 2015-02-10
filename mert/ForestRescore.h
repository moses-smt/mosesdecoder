/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

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
#ifndef MERT_FOREST_RESCORE_H
#define MERT_FOREST_RESCORE_H

#include <valarray>
#include <vector>

#include <boost/unordered_set.hpp>

#include "BleuScorer.h"
#include "Hypergraph.h"

namespace MosesTuning
{

std::ostream& operator<<(std::ostream& out, const WordVec& wordVec);

struct NgramHash : public std::unary_function<const WordVec&, std::size_t> {
  std::size_t operator()(const WordVec& ngram) const {
    return util::MurmurHashNative(&(ngram[0]), ngram.size() * sizeof(WordVec::value_type));
  }
};

struct NgramEquals : public std::binary_function<const WordVec&, const WordVec&, bool> {
  bool operator()(const WordVec& first, const WordVec& second) const {
    if (first.size() != second.size()) return false;
    return memcmp(&(first[0]), &(second[0]), first.size() * sizeof(WordVec::value_type)) == 0;
  }
};

typedef boost::unordered_map<WordVec, size_t, NgramHash, NgramEquals> NgramCounter;


class ReferenceSet
{


public:

  void AddLine(size_t sentenceId, const StringPiece& line, Vocab& vocab);

  void Load(const std::vector<std::string>& files, Vocab& vocab);

  size_t NgramMatches(size_t sentenceId, const WordVec&, bool clip) const;

  size_t Length(size_t sentenceId) const {
    return lengths_[sentenceId];
  }

private:
  //ngrams to (clipped,unclipped) counts
  typedef boost::unordered_map<WordVec, std::pair<std::size_t,std::size_t>, NgramHash,NgramEquals> NgramMap;
  std::vector<NgramMap> ngramCounts_;
  std::vector<size_t> lengths_;

};

struct VertexState {
  VertexState();

  std::vector<FeatureStatsType> bleuStats;
  WordVec leftContext;
  WordVec rightContext;
  size_t targetLength;
};

/**
  * Used to score an rule (ie edge) when we are applying it.
**/
class HgBleuScorer
{
public:
  HgBleuScorer(const ReferenceSet& references, const Graph& graph, size_t sentenceId, const std::vector<FeatureStatsType>& backgroundBleu):
    references_(references), sentenceId_(sentenceId), graph_(graph), backgroundBleu_(backgroundBleu),
    backgroundRefLength_(backgroundBleu[kBleuNgramOrder*2]) {
    vertexStates_.resize(graph.VertexSize());
    totalSourceLength_ = graph.GetVertex(graph.VertexSize()-1).SourceCovered();
  }

  FeatureStatsType Score(const Edge& edge, const Vertex& head, std::vector<FeatureStatsType>& bleuStats) ;

  void UpdateState(const Edge& winnerEdge, size_t vertexId, const std::vector<FeatureStatsType>& bleuStats);


private:
  const ReferenceSet& references_;
  std::vector<VertexState> vertexStates_;
  size_t sentenceId_;
  size_t totalSourceLength_;
  const Graph& graph_;
  std::vector<FeatureStatsType> backgroundBleu_;
  FeatureStatsType backgroundRefLength_;

  void UpdateMatches(const NgramCounter& counter, std::vector<FeatureStatsType>& bleuStats) const;
  size_t GetTargetLength(const Edge& edge) const;
};

struct HgHypothesis {
  SparseVector featureVector;
  WordVec text;
  std::vector<FeatureStatsType> bleuStats;
};

void Viterbi(const Graph& graph, const SparseVector& weights, float bleuWeight, const ReferenceSet& references, size_t sentenceId, const std::vector<FeatureStatsType>& backgroundBleu, HgHypothesis* bestHypo);

};

#endif
