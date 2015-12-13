#include "HypPackEnumerator.h"

#include <cassert>
#include <algorithm>
#include <boost/unordered_set.hpp>

using namespace std;

namespace MosesTuning
{


StreamingHypPackEnumerator::StreamingHypPackEnumerator
(
  vector<std::string> const& featureFiles,
  vector<std::string> const& scoreFiles
)
  : m_featureFiles(featureFiles),
    m_scoreFiles(scoreFiles)
{
  if (scoreFiles.size() == 0 || featureFiles.size() == 0) {
    cerr << "No data to process" << endl;
    exit(0);
  }

  if (featureFiles.size() != scoreFiles.size()) {
    cerr << "Error: Number of feature files (" << featureFiles.size() <<
         ") does not match number of score files (" << scoreFiles.size() << ")" << endl;
    exit(1);
  }

  m_num_lists = scoreFiles.size();
  m_primed = false;
  m_iNumDense = -1;
}

size_t StreamingHypPackEnumerator::num_dense() const
{
  if(m_iNumDense<0) {
    cerr << "Error: Requested num_dense() for an unprimed StreamingHypPackEnumerator" << endl;
    exit(1);
  }
  return (size_t) m_iNumDense;
}

void StreamingHypPackEnumerator::prime()
{
  m_current_indexes.clear();
  m_current_featureVectors.clear();
  boost::unordered_set<FeatureDataItem> seen;
  m_primed = true;

  for (size_t i = 0; i < m_num_lists; ++i) {
    if (m_featureDataIters[i] == FeatureDataIterator::end()) {
      cerr << "Error: Feature file " << i << " ended prematurely" << endl;
      exit(1);
    }
    if (m_scoreDataIters[i] == ScoreDataIterator::end()) {
      cerr << "Error: Score file " << i << " ended prematurely" << endl;
      exit(1);
    }
    if (m_featureDataIters[i]->size() != m_scoreDataIters[i]->size()) {
      cerr << "Error: For sentence " << m_sentenceId << " features and scores have different size" << endl;
      exit(1);
    }
    for (size_t j = 0; j < m_featureDataIters[i]->size(); ++j) {
      const FeatureDataItem& item = m_featureDataIters[i]->operator[](j);
      // Dedup
      if(seen.find(item)==seen.end()) {
        seen.insert(item);
        // Confirm dense features are always the same
        int iDense = item.dense.size();
        if(m_iNumDense != iDense) {
          if(m_iNumDense==-1) m_iNumDense = iDense;
          else {
            cerr << "Error: expecting constant number of dense features: "
                 << m_iNumDense << " != " << iDense << endl;
            exit(1);
          }
        }
        // Store item for retrieval
        m_current_indexes.push_back(pair<size_t,size_t>(i,j));
        m_current_featureVectors.push_back(MiraFeatureVector(item));
      }
    }
  }
}

void StreamingHypPackEnumerator::reset()
{
  m_featureDataIters.clear();
  m_scoreDataIters.clear();
  for (size_t i = 0; i < m_num_lists; ++i) {
    m_featureDataIters.push_back(FeatureDataIterator(m_featureFiles[i]));
    m_scoreDataIters.push_back(ScoreDataIterator(m_scoreFiles[i]));
  }
  m_sentenceId=0;
  prime();
}

bool StreamingHypPackEnumerator::finished()
{
  return m_featureDataIters[0]==FeatureDataIterator::end();
}

void StreamingHypPackEnumerator::next()
{
  if(!m_primed) {
    cerr << "Enumerating an unprimed HypPackEnumerator" << endl;
    exit(1);
  }
  for (size_t i = 0; i < m_num_lists; ++i) {
    ++m_featureDataIters[i];
    ++m_scoreDataIters[i];
  }
  m_sentenceId++;
  if(m_sentenceId % 100 == 0) cerr << ".";
  if(!finished()) prime();
}

size_t StreamingHypPackEnumerator::cur_size()
{
  if(!m_primed) {
    cerr << "Querying size from an unprimed HypPackEnumerator" << endl;
    exit(1);
  }
  return m_current_indexes.size();
}

const MiraFeatureVector& StreamingHypPackEnumerator::featuresAt(size_t index)
{
  if(!m_primed) {
    cerr << "Querying features from an unprimed HypPackEnumerator" << endl;
    exit(1);
  }
  return m_current_featureVectors[index];
}

const ScoreDataItem& StreamingHypPackEnumerator::scoresAt(size_t index)
{
  if(!m_primed) {
    cerr << "Querying scores from an unprimed HypPackEnumerator" << endl;
    exit(1);
  }
  const pair<size_t,size_t>& pij = m_current_indexes[index];
  return m_scoreDataIters[pij.first]->operator[](pij.second);
}

size_t StreamingHypPackEnumerator::cur_id()
{
  return m_sentenceId;
}

/* --------- RandomAccessHypPackEnumerator ------------- */

RandomAccessHypPackEnumerator::RandomAccessHypPackEnumerator(vector<string> const& featureFiles,
    vector<string> const& scoreFiles,
    bool no_shuffle)
{
  StreamingHypPackEnumerator train(featureFiles,scoreFiles);
  size_t index=0;
  for(train.reset(); !train.finished(); train.next()) {
    m_features.push_back(vector<MiraFeatureVector>());
    m_scores.push_back(vector<ScoreDataItem>());
    for(size_t j=0; j<train.cur_size(); j++) {
      m_features.back().push_back(train.featuresAt(j));
      m_scores.back().push_back(train.scoresAt(j));
    }
    m_indexes.push_back(index++);
  }

  m_cur_index = 0;
  m_no_shuffle = no_shuffle;
  m_num_dense = train.num_dense();
}

size_t RandomAccessHypPackEnumerator::num_dense() const
{
  return m_num_dense;
}

void RandomAccessHypPackEnumerator::reset()
{
  m_cur_index = 0;
  if(!m_no_shuffle) random_shuffle(m_indexes.begin(),m_indexes.end());
}
bool RandomAccessHypPackEnumerator::finished()
{
  return m_cur_index >= m_indexes.size();
}
void RandomAccessHypPackEnumerator::next()
{
  m_cur_index++;
}

size_t RandomAccessHypPackEnumerator::cur_size()
{
  assert(m_features[m_indexes[m_cur_index]].size()==m_scores[m_indexes[m_cur_index]].size());
  return m_features[m_indexes[m_cur_index]].size();
}
const MiraFeatureVector& RandomAccessHypPackEnumerator::featuresAt(size_t i)
{
  return m_features[m_indexes[m_cur_index]][i];
}
const ScoreDataItem& RandomAccessHypPackEnumerator::scoresAt(size_t i)
{
  return m_scores[m_indexes[m_cur_index]][i];
}

size_t RandomAccessHypPackEnumerator::cur_id()
{
  return m_indexes[m_cur_index];
}
// --Emacs trickery--
// Local Variables:
// mode:c++
// c-basic-offset:2
// End:

}
