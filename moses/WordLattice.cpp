#include <map>
#include "StaticData.h"
#include "WordLattice.h"
#include "PCNTools.h"
#include "Util.h"
#include "FloydWarshall.h"
#include "TranslationOptionCollectionLattice.h"
#include "TranslationOptionCollectionConfusionNet.h"
#include "moses/FF/InputFeature.h"
#include "moses/TranslationTask.h"

namespace Moses
{
WordLattice::WordLattice(AllOptions::ptr const& opts)  : ConfusionNet(opts)
{
  UTIL_THROW_IF2(InputFeature::InstancePtr() == NULL,
                 "Input feature must be specified");
}

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

      // dense
      std::vector<float>::const_iterator iterDense;
      for(iterDense = data[i][j].second.denseScores.begin(); iterDense < data[i][j].second.denseScores.end(); ++iterDense) {
        out<<", "<<*iterDense;
      }

      // sparse
      std::map<StringPiece, float>::const_iterator iterSparse;
      for(iterSparse = data[i][j].second.sparseScores.begin(); iterSparse != data[i][j].second.sparseScores.end(); ++iterSparse) {
        out << ", " << iterSparse->first << "=" << iterSparse->second;
      }

      out << GetColumnIncrement(i,j) << ") ";
    }

    out<<"\n";
  }
  out<<"\n\n";
}

int
WordLattice::
InitializeFromPCNDataType(const PCN::CN& cn, const std::string& debug_line)
{
  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;
  size_t const maxPhraseLength = m_options->search.max_phrase_length;

  const InputFeature *inputFeature = InputFeature::InstancePtr();
  size_t numInputScores = inputFeature->GetNumInputScores();
  size_t numRealWordCount = inputFeature->GetNumRealWordsInInput();

  bool addRealWordCount = (numRealWordCount > 0);

  //when we have one more weight than params, we add a word count feature
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
      if (alt.m_denseFeatures.size() != numInputScores) {
        TRACE_ERR("ERROR: need " << numInputScores
                  << " link parameters, found "
                  << alt.m_denseFeatures.size()
                  << " while reading column " << i
                  << " from " << debug_line << "\n");
        return false;
      }

      //check each element for bounds
      std::vector<float>::const_iterator probsIterator;
      data[i][j].second = std::vector<float>(0);
      for(probsIterator = alt.m_denseFeatures.begin();
          probsIterator < alt.m_denseFeatures.end();
          probsIterator++) {
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

        float score = std::max(static_cast<float>(log(*probsIterator)), LOWEST_SCORE);
        ScorePair &scorePair = data[i][j].second;
        scorePair.denseScores.push_back(score);
      }
      //store 'real' word count in last feature if we have one more weight than we do arc scores and not epsilon
      if (addRealWordCount) {
        //only add count if not epsilon
        float value = (alt.m_word=="" || alt.m_word==EPSILON) ? 0.0f : -1.0f;
        data[i][j].second.denseScores.push_back(value);
      }
      Word& w = data[i][j].first;
      w.CreateFromString(Input,factorOrder,StringPiece(alt.m_word),false);
      // String2Word(alt.m_word, data[i][j]. first, factorOrder);
      next_nodes[i][j] = alt.m_next;

      if(next_nodes[i][j] > maxPhraseLength) {
        TRACE_ERR("ERROR: Jump length " << next_nodes[i][j] << " in word lattice exceeds maximum phrase length " << maxPhraseLength << ".\n");
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

int
WordLattice::
Read(std::istream& in)
{
  Clear();
  std::string line;
  if(!getline(in,line)) return 0;
  std::map<std::string, std::string> meta=ProcessAndStripSGML(line);
  if (meta.find("id") != meta.end()) {
    this->SetTranslationId(atol(meta["id"].c_str()));
  }

  PCN::CN cn = PCN::parsePCN(line);
  return InitializeFromPCNDataType(cn, line);
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

int WordLattice::ComputeDistortionDistance(const Range& prev, const Range& current) const
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

TranslationOptionCollection*
WordLattice::
CreateTranslationOptionCollection(ttasksptr const& ttask) const
{
  TranslationOptionCollection *rv = NULL;
  if (StaticData::Instance().GetUseLegacyPT()) {
    rv = new TranslationOptionCollectionConfusionNet(ttask, *this);
  } else {
    rv = new TranslationOptionCollectionLattice(ttask, *this);
  }
  assert(rv);
  return rv;
}


std::ostream& operator<<(std::ostream &out, const WordLattice &obj)
{
  out << "next_nodes=";
  for (size_t i = 0; i < obj.next_nodes.size(); ++i) {
    out << i << ":";

    const std::vector<size_t> &inner = obj.next_nodes[i];
    for (size_t j = 0; j < inner.size(); ++j) {
      out << inner[j] << " ";
    }
  }

  out << "distances=";
  for (size_t i = 0; i < obj.distances.size(); ++i) {
    out << i << ":";

    const std::vector<int> &inner = obj.distances[i];
    for (size_t j = 0; j < inner.size(); ++j) {
      out << inner[j] << " ";
    }
  }
  return out;
}

} // namespace

