/*
 *  Data.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <algorithm>
#include <cmath>
#include <fstream>

#include "Data.h"
#include "FileStream.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "Util.h"
#include "util/check.hh"

using namespace std;

namespace MosesTuning
{
  

Data::Data()
  : m_scorer(NULL),
    m_num_scores(0),
    m_sparse_flag(false),
    m_score_data(),
    m_feature_data() {}

Data::Data(Scorer* scorer)
    : m_scorer(scorer),
      m_score_type(m_scorer->getName()),
      m_num_scores(0),
      m_sparse_flag(false),
      m_score_data(new ScoreData(m_scorer)),
      m_feature_data(new FeatureData)
{
  TRACE_ERR("Data::m_score_type " << m_score_type << endl);
  TRACE_ERR("Data::Scorer type from Scorer: " << m_scorer->getName() << endl);
}

//ADDED BY TS
// TODO: This is too long; consider creating additional functions to
// reduce the lines of this function.
void Data::removeDuplicates() {
  size_t nSentences = m_feature_data->size();
  assert(m_score_data->size() == nSentences);

  for (size_t s = 0; s < nSentences; s++) {
    FeatureArray& feat_array =  m_feature_data->get(s);
    ScoreArray& score_array =  m_score_data->get(s);

    assert(feat_array.size() == score_array.size());

    //serves as a hash-map:
    map<double, vector<size_t> > lookup;

    size_t end_pos = feat_array.size() - 1;

    size_t nRemoved = 0;

    for (size_t k = 0; k <= end_pos; k++) {
      const FeatureStats& cur_feats = feat_array.get(k);
      double sum = 0.0;
      for (size_t l = 0; l < cur_feats.size(); l++)
        sum += cur_feats.get(l);

      if (lookup.find(sum) != lookup.end()) {

        //cerr << "hit" << endl;
        vector<size_t>& cur_list = lookup[sum];

        // TODO: Make sure this is correct because we have already used 'l'.
        // If this does not impact on the removing duplicates, it is better
        // to change
        size_t l = 0;
        for (l = 0; l < cur_list.size(); l++) {
          size_t j = cur_list[l];

          if (cur_feats == feat_array.get(j)
              && score_array.get(k) == score_array.get(j)) {
            if (k < end_pos) {
              feat_array.swap(k,end_pos);
              score_array.swap(k,end_pos);
              k--;
            }
            end_pos--;
            nRemoved++;
            break;
          }
        }
        if (l == lookup[sum].size())
          cur_list.push_back(k);
      } else {
        lookup[sum].push_back(k);
      }
      // for (size_t j=0; j < k; j++) {

      // 	if (feat_array.get(k) == feat_array.get(j)
      // 	    && score_array.get(k) == score_array.get(j)) {

      // 	  if (k < end_pos) {

      // 	    feat_array.swap(k,end_pos);
      // 	    score_array.swap(k,end_pos);

      // 	    k--;
      // 	  }

      //          end_pos--;
      // 	  nRemoved++;
      //          break;
      // 	}
      // }
    } // end for k

    if (nRemoved > 0) {
      feat_array.resize(end_pos+1);
      score_array.resize(end_pos+1);
    }
  }
}
//END_ADDED

void Data::load(const std::string &featfile, const std::string &scorefile) {
  m_feature_data->load(featfile);
  m_score_data->load(scorefile);
  if (m_feature_data->hasSparseFeatures())
    m_sparse_flag = true;
}

void Data::loadNBest(const string &file)
{
  TRACE_ERR("loading nbest from " << file << endl);
  inputfilestream inp(file); // matches a stream with a file. Opens the file
  if (!inp.good())
    throw runtime_error("Unable to open: " + file);

  ScoreStats scoreentry;
  string line, sentence_index, sentence, feature_str, alignment;

  while (getline(inp, line, '\n')) {
    if (line.empty()) continue;
    // adding statistics for error measures
    scoreentry.clear();

    getNextPound(line, sentence_index, "|||"); // first field
    getNextPound(line, sentence, "|||");       // second field
    getNextPound(line, feature_str, "|||");    // third field

    if (line.length() > 0) {
      string temp;
      getNextPound(line, temp, "|||"); //fourth field sentence score
      if (line.length() > 0) {
        getNextPound(line, alignment, "|||"); //fourth field only there if alignment scorer
      }
    }
    //TODO check alignment exists if scorers need it

    if (m_scorer->useAlignment()) {
      sentence += "|||";
      sentence += alignment;
    }
    m_scorer->prepareStats(sentence_index, sentence, scoreentry);
    
    m_score_data->add(scoreentry, sentence_index);

    // examine first line for name of features
    if (!existsFeatureNames()) {
      InitFeatureMap(feature_str);
    }
    AddFeatures(feature_str, sentence_index);
  }
  inp.close();
}

void Data::save(const std::string &featfile, const std::string &scorefile, bool bin) {
  if (bin)
    cerr << "Binary write mode is selected" << endl;
  else
    cerr << "Binary write mode is NOT selected" << endl;

  m_feature_data->save(featfile, bin);
  m_score_data->save(scorefile, bin);
}

void Data::InitFeatureMap(const string& str) {
  string buf = str;
  string substr;
  string features = "";
  string tmp_name = "";
  size_t tmp_index = 0;

  while (!buf.empty()) {
    getNextPound(buf, substr);

    // string ending with ":" are skipped, because they are the names of the features
    if (!EndsWith(substr, ":")) {
      stringstream ss;
      ss << tmp_name << "_" << tmp_index << " ";
      features.append(ss.str());

      tmp_index++;
    } else if (substr.find("_") != string::npos) {
      // ignore sparse feature name and its value
      getNextPound(buf, substr);
    } else {                              // update current feature name
      tmp_index = 0;
      tmp_name = substr.substr(0, substr.size() - 1);
    }
  }
  m_feature_data->setFeatureMap(features);
}

void Data::AddFeatures(const string& str,
                       const string& sentence_index) {
  string buf = str;
  string substr;
  FeatureStats feature_entry;
  feature_entry.reset();

  while (!buf.empty()) {
    getNextPound(buf, substr);

    // no ':' -> feature value that needs to be stored
    if (!EndsWith(substr, ":")) {
      feature_entry.add(ConvertStringToFeatureStatsType(substr));
    } else if (substr.find("_") != string::npos) {
      // sparse feature name? store as well
      string name = substr;
      getNextPound(buf, substr);
      feature_entry.addSparse(name, atof(substr.c_str()));
      m_sparse_flag = true;
    }
  }
  m_feature_data->add(feature_entry, sentence_index);
}

// TODO
void Data::mergeSparseFeatures() {
  cerr << "ERROR: sparse features can only be trained with pairwise ranked optimizer (PRO), not traditional MERT\n";
  exit(1);
}

void Data::createShards(size_t shard_count, float shard_size, const string& scorerconfig,
                        vector<Data>& shards)
{
  CHECK(shard_count);
  CHECK(shard_size >= 0);
  CHECK(shard_size <= 1);

  size_t data_size = m_score_data->size();
  CHECK(data_size == m_feature_data->size());

  shard_size *= data_size;
  const float coeff = static_cast<float>(data_size) / shard_count;

  for (size_t shard_id = 0; shard_id < shard_count; ++shard_id) {
    vector<size_t> shard_contents;
    if (shard_size == 0) {
      //split into roughly equal size shards
      const size_t shard_start = floor(0.5 + shard_id * coeff);
      const size_t shard_end = floor(0.5 + (shard_id + 1) * coeff);
      for (size_t i = shard_start; i < shard_end; ++i) {
        shard_contents.push_back(i);
      }
    } else {
      //create shards by randomly sampling
      for (size_t i = 0; i < floor(shard_size+0.5); ++i) {
        shard_contents.push_back(rand() % data_size);
      }
    }

    Scorer* scorer = ScorerFactory::getScorer(m_score_type, scorerconfig);

    shards.push_back(Data(scorer));
    shards.back().m_score_type = m_score_type;
    shards.back().m_num_scores = m_num_scores;
    shards.back().m_sparse_flag = m_sparse_flag;
    for (size_t i = 0; i < shard_contents.size(); ++i) {
      shards.back().m_feature_data->add(m_feature_data->get(shard_contents[i]));
      shards.back().m_score_data->add(m_score_data->get(shard_contents[i]));
    }
    //cerr << endl;
  }
}

}

