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
#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>

#include "util/double-conversion/double-conversion.h"
#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "Hypergraph.h"

using namespace std;
static const string kBOS = "<s>";
static const string kEOS = "</s>";

namespace MosesTuning
{

StringPiece NextLine(util::FilePiece& from)
{
  StringPiece line;
  while ((line = from.ReadLine()).starts_with("#"));
  return line;
}

Vocab::Vocab() :  eos_( FindOrAdd(kEOS)), bos_(FindOrAdd(kBOS))
{
}

const Vocab::Entry &Vocab::FindOrAdd(const StringPiece &str)
{
#if BOOST_VERSION >= 104200
  Map::const_iterator i= map_.find(str, Hash(), Equals());
#else
  std::string copied_str(str.data(), str.size());
  Map::const_iterator i = map_.find(copied_str.c_str());
#endif
  if (i != map_.end()) return *i;
  char *copied = static_cast<char*>(piece_backing_.Allocate(str.size() + 1));
  memcpy(copied, str.data(), str.size());
  copied[str.size()] = 0;
  return *map_.insert(Entry(copied, map_.size())).first;
}

double_conversion::StringToDoubleConverter converter(double_conversion::StringToDoubleConverter::NO_FLAGS, NAN, NAN, "inf", "nan");


/**
 * Reads an incoming edge. Returns edge and source words covered.
**/
static pair<Edge*,size_t> ReadEdge(util::FilePiece &from, Graph &graph)
{
  Edge* edge = graph.NewEdge();
  StringPiece line = from.ReadLine(); //Don't allow comments within edge lists
  util::TokenIter<util::MultiCharacter> pipes(line, util::MultiCharacter(" ||| "));
  //Target
  for (util::TokenIter<util::SingleCharacter, true> i(*pipes, util::SingleCharacter(' ')); i; ++i) {
    StringPiece got = *i;
    if ('[' == *got.data() && ']' == got.data()[got.size() - 1]) {
      // non-terminal
      char *end_ptr;
      unsigned long int child = std::strtoul(got.data() + 1, &end_ptr, 10);
      UTIL_THROW_IF(end_ptr != got.data() + got.size() - 1, HypergraphException, "Bad non-terminal" << got);
      UTIL_THROW_IF(child >= graph.VertexSize(), HypergraphException, "Reference to vertex " << child << " but we only have " << graph.VertexSize() << " vertices.  Is the file in bottom-up format?");
      edge->AddWord(NULL);
      edge->AddChild(child);
    } else {
      const Vocab::Entry &found = graph.MutableVocab().FindOrAdd(got);
      edge->AddWord(&found);
    }
  }

  //Features
  ++pipes;
  for (util::TokenIter<util::SingleCharacter, true> i(*pipes, util::SingleCharacter(' ')); i; ++i) {
    StringPiece fv = *i;
    if (!fv.size()) break;
    size_t equals = fv.find_last_of("=");
    UTIL_THROW_IF(equals == fv.npos, HypergraphException, "Failed to parse feature '" << fv << "'");
    StringPiece name = fv.substr(0,equals);
    StringPiece value = fv.substr(equals+1);
    int processed;
    float score = converter.StringToFloat(value.data(), value.length(), &processed);
    UTIL_THROW_IF(isnan(score), HypergraphException, "Failed to parse weight '" << value << "'");
    edge->AddFeature(name,score);
  }
  //Covered words
  ++pipes;
  size_t sourceCovered = boost::lexical_cast<size_t>(*pipes);
  return pair<Edge*,size_t>(edge,sourceCovered);
}

void Graph::Prune(Graph* pNewGraph, const SparseVector& weights, size_t minEdgeCount) const
{

  Graph& newGraph = *pNewGraph;
  //TODO: Optimise case where no pruning required

  //For debug


  /*
  map<const Edge*, string> edgeIds;
  for (size_t i = 0; i < edges_.Size(); ++i) {
    stringstream str;
    size_t childId = 0;
    for (size_t j = 0; j < edges_[i].Words().size(); ++j) {
      if (edges_[i].Words()[j]) {
        str << edges_[i].Words()[j]->first << " ";
      } else {
        str << "[" << edges_[i].Children()[childId++] << "] ";
      }
    }
    edgeIds[&(edges_[i])] = str.str();
  }
  */

  //end For debug

  map<const Edge*, FeatureStatsType> edgeBackwardScores;
  map<const Edge*, size_t> edgeHeads;
  vector<FeatureStatsType> vertexBackwardScores(vertices_.Size(), kMinScore);
  vector<vector<const Edge*> > outgoing(vertices_.Size());

  //Compute backward scores
  for (size_t vi = 0; vi < vertices_.Size(); ++vi) {
    // cerr << "Vertex " << vi << endl;
    const Vertex& vertex = vertices_[vi];
    const vector<const Edge*>& incoming = vertex.GetIncoming();
    if (!incoming.size()) {
      vertexBackwardScores[vi] = 0;
    } else {
      for (size_t ei = 0; ei < incoming.size(); ++ei) {
        //cerr << "Edge " << edgeIds[incoming[ei]] << endl;
        edgeHeads[incoming[ei]]= vi;
        FeatureStatsType incomingScore = incoming[ei]->GetScore(weights);
        for (size_t i = 0; i < incoming[ei]->Children().size(); ++i) {
          //cerr << "\tChild " << incoming[ei]->Children()[i] << endl;
          size_t childId = incoming[ei]->Children()[i];
          UTIL_THROW_IF(vertexBackwardScores[childId] == kMinScore,
                        HypergraphException, "Graph was not topologically sorted. curr=" << vi << " prev=" << childId);
          outgoing[childId].push_back(incoming[ei]);
          incomingScore += vertexBackwardScores[childId];
        }
        edgeBackwardScores[incoming[ei]]= incomingScore;
        //cerr << "Backward score: " << incomingScore << endl;
        if (incomingScore > vertexBackwardScores[vi]) vertexBackwardScores[vi] = incomingScore;
      }
    }
  }

  //Compute forward scores
  vector<FeatureStatsType> vertexForwardScores(vertices_.Size(), kMinScore);
  map<const Edge*, FeatureStatsType> edgeForwardScores;
  for (size_t i = 1; i <= vertices_.Size(); ++i) {
    size_t vi = vertices_.Size() - i;
    //cerr << "Vertex " << vi << endl;
    if (!outgoing[vi].size()) {
      vertexForwardScores[vi] = 0;
    } else {
      for (size_t ei = 0; ei < outgoing[vi].size(); ++ei) {
        //cerr << "Edge " << edgeIds[outgoing[vi][ei]] << endl;
        FeatureStatsType outgoingScore = 0;
        //add score of head
        outgoingScore += vertexForwardScores[edgeHeads[outgoing[vi][ei]]];
        //cerr << "Forward score " << outgoingScore << endl;
        edgeForwardScores[outgoing[vi][ei]] = outgoingScore;
        //sum scores of siblings
        for (size_t i = 0; i < outgoing[vi][ei]->Children().size(); ++i) {
          size_t siblingId = outgoing[vi][ei]->Children()[i];
          if (siblingId != vi) {
            //cerr << "\tSibling " << siblingId << endl;
            outgoingScore += vertexBackwardScores[siblingId];
          }
        }
        outgoingScore += outgoing[vi][ei]->GetScore(weights);
        if (outgoingScore > vertexForwardScores[vi]) vertexForwardScores[vi] = outgoingScore;
        //cerr << "Vertex " << vi << " forward score " << outgoingScore << endl;
      }
    }
  }



  multimap<FeatureStatsType, const Edge*> edgeScores;
  for (size_t i = 0; i < edges_.Size(); ++i) {
    const Edge* edge = &(edges_[i]);
    if (edgeForwardScores.find(edge) == edgeForwardScores.end()) {
      //This edge has no children, so didn't get a forward score. Its forward score
      //is that of its head
      edgeForwardScores[edge] = vertexForwardScores[edgeHeads[edge]];
    }
    FeatureStatsType score = edgeForwardScores[edge] + edgeBackwardScores[edge];
    edgeScores.insert(pair<FeatureStatsType, const Edge*>(score,edge));
    //  cerr << edgeIds[edge] << " " << score << endl;
  }



  multimap<FeatureStatsType, const Edge*>::const_reverse_iterator ei = edgeScores.rbegin();
  size_t edgeCount = 1;
  while(edgeCount < minEdgeCount && ei != edgeScores.rend()) {
    ++ei;
    ++edgeCount;
  }
  multimap<FeatureStatsType, const Edge*>::const_iterator lowest = edgeScores.begin();
  if (ei != edgeScores.rend())  lowest = edgeScores.lower_bound(ei->first);

  //cerr << "Retained edges" << endl;
  set<size_t> retainedVertices;
  set<const Edge*> retainedEdges;
  for (; lowest != edgeScores.end(); ++lowest) {
    //cerr << lowest->first << " " << edgeIds[lowest->second] << endl;
    retainedEdges.insert(lowest->second);
    retainedVertices.insert(edgeHeads[lowest->second]);
    for (size_t i = 0; i < lowest->second->Children().size(); ++i) {
      retainedVertices.insert(lowest->second->Children()[i]);
    }
  }
  newGraph.SetCounts(retainedVertices.size(), retainedEdges.size());

  //cerr << "Retained vertices" << endl;
  map<size_t,size_t> oldIdToNew;
  size_t vi = 0;
  for (set<size_t>::const_iterator i = retainedVertices.begin(); i != retainedVertices.end(); ++i, ++vi) {
//   cerr << *i << " New: " << vi << endl;
    oldIdToNew[*i] = vi;
    Vertex* vertex = newGraph.NewVertex();
    vertex->SetSourceCovered(vertices_[*i].SourceCovered());
  }

  for (set<const Edge*>::const_iterator i = retainedEdges.begin(); i != retainedEdges.end(); ++i) {
    Edge* newEdge = newGraph.NewEdge();
    const Edge* oldEdge = *i;
    for (size_t j = 0; j < oldEdge->Words().size(); ++j) {
      newEdge->AddWord(oldEdge->Words()[j]);
    }
    for (size_t j = 0; j < oldEdge->Children().size(); ++j) {
      newEdge->AddChild(oldIdToNew[oldEdge->Children()[j]]);
    }
    newEdge->SetFeatures(oldEdge->Features());
    Vertex& newHead = newGraph.vertices_[oldIdToNew[edgeHeads[oldEdge]]];
    newHead.AddEdge(newEdge);
  }


  /*
  cerr << "New graph" << endl;
  for (size_t vi = 0; vi < newGraph.VertexSize(); ++vi) {
    cerr << "Vertex " << vi << endl;
    const vector<const Edge*> incoming = newGraph.GetVertex(vi).GetIncoming();
    for (size_t ei = 0; ei < incoming.size(); ++ei) {
      size_t childId = 0;
      for (size_t wi = 0; wi < incoming[ei]->Words().size(); ++wi) {
        const Vocab::Entry* word = incoming[ei]->Words()[wi];
        if (word) {
          cerr << word->first << " ";
        } else {
          cerr << "[" << incoming[ei]->Children()[childId++] << "] ";
        }
      }
      cerr << " Score: " << incoming[ei]->GetScore(weights) <<  endl;
    }
    cerr << endl;
  }

  */

}

/**
  * Read from "Kenneth's hypergraph" aka cdec target_graph format (with comments)
**/
void ReadGraph(util::FilePiece &from, Graph &graph)
{

  //First line should contain field names
  StringPiece line = from.ReadLine();
  UTIL_THROW_IF(line.compare("# target ||| features ||| source-covered") != 0, HypergraphException, "Incorrect format spec on first line: '" << line << "'");
  line = NextLine(from);

  //Then expect numbers of vertices
  util::TokenIter<util::SingleCharacter, false> i(line, util::SingleCharacter(' '));
  unsigned long int vertices = boost::lexical_cast<unsigned long int>(*i);
  ++i;
  unsigned long int edges = boost::lexical_cast<unsigned long int>(*i);
  graph.SetCounts(vertices, edges);
  //cerr << "vertices: " << vertices << "; edges: " << edges << endl;
  for (size_t i = 0; i < vertices; ++i) {
    line = NextLine(from);
    unsigned long int edge_count = boost::lexical_cast<unsigned long int>(line);
    Vertex* vertex = graph.NewVertex();
    for (unsigned long int e = 0; e < edge_count; ++e) {
      pair<Edge*,size_t> edge = ReadEdge(from, graph);
      vertex->AddEdge(edge.first);
      //Note: the file format attaches this to the edge, but it's really a property
      //of the vertex.
      if (!e) {
        vertex->SetSourceCovered(edge.second);
      }
    }
  }
}

};
