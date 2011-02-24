#include "StaticData.h"
#include "WordLattice.h"
#include "PCNTools.h"
#include "Util.h"
#include "FloydWarshall.h"

namespace Moses
{
WordLattice::WordLattice() {}

size_t WordLattice::GetColumnIncrement(size_t i, size_t j) const
{
  return next_nodes[i][j];
}

void WordLattice::Print(std::ostream& out) const
{
  out<<"word lattice: "<<data.size()<<"\n";
  for(size_t i=0; i<data.size(); ++i) {
    out<<i<<" -- ";
    for(size_t j=0; j<data[i].size(); ++j) {
      out<<"("<<data[i][j].first.ToString()<<", ";
      for(std::vector<float>::const_iterator scoreIterator = data[i][j].second.begin(); scoreIterator<data[i][j].second.end(); scoreIterator++) {
        out<<*scoreIterator<<", ";
      }
      out << GetColumnIncrement(i,j) << ") ";
    }

    out<<"\n";
  }
  out<<"\n\n";
}

int WordLattice::InitializeFromPCNDataType(const PCN::CN& cn, const std::vector<FactorType>& factorOrder, const std::string& debug_line)
{
  size_t numLinkParams = StaticData::Instance().GetNumLinkParams();
  size_t numLinkWeights = StaticData::Instance().GetNumInputScores();
  size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();

  //when we have one more weight than params, we add a word count feature
  bool addRealWordCount = ((numLinkParams + 1) == numLinkWeights);
  data.resize(cn.size());
  next_nodes.resize(cn.size());
  for(size_t i=0; i<cn.size(); ++i) {
    const PCN::CNCol& col = cn[i];
    if (col.empty()) return false;
    data[i].resize(col.size());
    next_nodes[i].resize(col.size());
    for (size_t j=0; j<col.size(); ++j) {
      const PCN::CNAlt& alt = col[j];


      //check for correct number of link parameters
      if (alt.first.second.size() != numLinkParams) {
        TRACE_ERR("ERROR: need " << numLinkParams << " link parameters, found " << alt.first.second.size() << " while reading column " << i << " from " << debug_line << "\n");
        return false;
      }

      //check each element for bounds
      std::vector<float>::const_iterator probsIterator;
      data[i][j].second = std::vector<float>(0);
      for(probsIterator = alt.first.second.begin(); probsIterator < alt.first.second.end(); probsIterator++) {
        IFVERBOSE(1) {
          if (*probsIterator < 0.0f) {
            TRACE_ERR("WARN: neg probability: " << *probsIterator << "\n");
            //*probsIterator = 0.0f;
          }
          if (*probsIterator > 1.0f) {
            TRACE_ERR("WARN: probability > 1: " << *probsIterator << "\n");
            //*probsIterator = 1.0f;
          }
        }
        data[i][j].second.push_back(std::max(static_cast<float>(log(*probsIterator)), LOWEST_SCORE));
      }
      //store 'real' word count in last feature if we have one more weight than we do arc scores and not epsilon
      if (addRealWordCount) {
        //only add count if not epsilon
        float value = (alt.first.first=="" || alt.first.first==EPSILON) ? 0.0f : -1.0f;
        data[i][j].second.push_back(value);
      }
      String2Word(alt.first.first,data[i][j].first,factorOrder);
      next_nodes[i][j] = alt.second;

      if(next_nodes[i][j] > maxSizePhrase) {
        TRACE_ERR("ERROR: Jump length " << next_nodes[i][j] << " in word lattice exceeds maximum phrase length " << maxSizePhrase << ".\n");
        TRACE_ERR("ERROR: Increase max-phrase-length to process this lattice.\n");
        return false;
      }
    }
  }
  if (!cn.empty()) {
    std::vector<std::vector<bool> > edges(0);
    this->GetAsEdgeMatrix(edges);
    floyd_warshall(edges,distances);

    IFVERBOSE(2) {
      TRACE_ERR("Shortest paths:\n");
      for (size_t i=0; i<edges.size(); ++i) {
        for (size_t j=0; j<edges.size(); ++j) {
          int d = distances[i][j];
          if (d > 99999) {
            d=-1;
          }
          TRACE_ERR("\t" << d);
        }
        TRACE_ERR("\n");
      }
    }
  }
  return !cn.empty();
}

int WordLattice::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
  Clear();
  std::string line;
  if(!getline(in,line)) return 0;
  std::map<std::string, std::string> meta=ProcessAndStripSGML(line);
  if (meta.find("id") != meta.end()) {
    this->SetTranslationId(atol(meta["id"].c_str()));
  }

  PCN::CN cn = PCN::parsePCN(line);
  return InitializeFromPCNDataType(cn, factorOrder, line);
}

void WordLattice::GetAsEdgeMatrix(std::vector<std::vector<bool> >& edges) const
{
  edges.resize(data.size()+1,std::vector<bool>(data.size()+1, false));
  for (size_t i=0; i<data.size(); ++i) {
    for (size_t j=0; j<data[i].size(); ++j) {
      edges[i][i+next_nodes[i][j]] = true;
    }
  }
}

int WordLattice::ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) const
{
  int result;

  if (prev.GetStartPos() == NOT_FOUND && current.GetStartPos() == 0) {
    result = 0;

    VERBOSE(4, "Word lattice distortion: monotonic initial step\n");
  } else if (prev.GetEndPos()+1 == current.GetStartPos()) {
    result = 0;

    VERBOSE(4, "Word lattice distortion: monotonic step from " << prev.GetEndPos() << " to " << current.GetStartPos() << "\n");
  } else if (prev.GetStartPos() == NOT_FOUND) {
    result = distances[0][current.GetStartPos()];

    VERBOSE(4, "Word lattice distortion: initial step from 0 to " << current.GetStartPos() << " of length " << result << "\n");
    if (result < 0 || result > 99999) {
      TRACE_ERR("prev: " << prev << "\ncurrent: " << current << "\n");
      TRACE_ERR("A: got a weird distance from 0 to " << (current.GetStartPos()+1) << " of " << result << "\n");
    }
  } else if (prev.GetEndPos() > current.GetStartPos()) {
    result = distances[current.GetStartPos()][prev.GetEndPos() + 1];

    VERBOSE(4, "Word lattice distortion: backward step from " << (prev.GetEndPos()+1) << " to " << current.GetStartPos() << " of length " << result << "\n");
    if (result < 0 || result > 99999) {
      TRACE_ERR("prev: " << prev << "\ncurrent: " << current << "\n");
      TRACE_ERR("B: got a weird distance from "<< current.GetStartPos() << " to " << prev.GetEndPos()+1 << " of " << result << "\n");
    }
  } else {
    result = distances[prev.GetEndPos() + 1][current.GetStartPos()];

    VERBOSE(4, "Word lattice distortion: forward step from " << (prev.GetEndPos()+1) << " to " << current.GetStartPos() << " of length " << result << "\n");
    if (result < 0 || result > 99999) {
      TRACE_ERR("prev: " << prev << "\ncurrent: " << current << "\n");
      TRACE_ERR("C: got a weird distance from "<< prev.GetEndPos()+1 << " to " << current.GetStartPos() << " of " << result << "\n");
    }
  }

  return result;
}

bool WordLattice::CanIGetFromAToB(size_t start, size_t end) const
{
  //  std::cerr << "CanIgetFromAToB(" << start << "," << end << ")=" << distances[start][end] << std::endl;
  return distances[start][end] < 100000;
}


}

