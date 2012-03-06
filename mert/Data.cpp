/*
 *  Data.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include <algorithm>
#include "util/check.hh"
#include <cmath>
#include <fstream>

#include "Data.h"
#include "FileStream.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "Util.h"

Data::Data()
  : theScorer(NULL),
    number_of_scores(0),
    _sparse_flag(false),
    scoredata(),
    featdata() {}

Data::Data(Scorer& ptr)
    : theScorer(&ptr),
      score_type(theScorer->getName()),
      number_of_scores(0),
      _sparse_flag(false),
      scoredata(new ScoreData(*theScorer)),
      featdata(new FeatureData)
{
  TRACE_ERR("Data::score_type " << score_type << std::endl);
  TRACE_ERR("Data::Scorer type from Scorer: " << theScorer->getName() << endl);
}

//ADDED BY TS
void Data::remove_duplicates() {

  size_t nSentences = featdata->size();
  assert(scoredata->size() == nSentences);

  for (size_t s=0; s < nSentences; s++) {

    FeatureArray& feat_array =  featdata->get(s);
    ScoreArray& score_array =  scoredata->get(s);

    assert(feat_array.size() == score_array.size());

    //serves as a hash-map:
    std::map<double, std::vector<size_t> > lookup;

    size_t end_pos = feat_array.size() - 1;

    size_t nRemoved = 0;
    for (size_t k=0; k <= end_pos; k++) {

      const FeatureStats& cur_feats = feat_array.get(k);

      double sum = 0.0;
      for (size_t l=0; l < cur_feats.size(); l++)
	sum += cur_feats.get(l);

      if (lookup.find(sum) != lookup.end()) {

	//std::cerr << "hit" << std::endl;

	std::vector<size_t>& cur_list = lookup[sum];

	size_t l=0;
	for (l=0; l < cur_list.size(); l++) {

	  size_t j=cur_list[l];

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
      }
      else
	lookup[sum].push_back(k);

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
    }


    if (nRemoved > 0) {

      feat_array.resize(end_pos+1);
      score_array.resize(end_pos+1);
    }
  }
}
//END_ADDED


void Data::loadnbest(const std::string &file)
{
  TRACE_ERR("loading nbest from " << file << std::endl);

  ScoreStats scoreentry;

  inputfilestream inp(file); // matches a stream with a file. Opens the file

  if (!inp.good())
    throw runtime_error("Unable to open: " + file);

  std::string subsubstring, stringBuf;
  std::string sentence_index, sentence, feature_str;
  std::string::size_type loc;

  while (getline(inp,stringBuf,'\n')) {
    if (stringBuf.empty()) continue;
    // adding statistics for error measures
    scoreentry.clear();

    getNextPound(stringBuf, sentence_index, "|||"); // first field
    getNextPound(stringBuf, sentence, "|||");       // second field
    getNextPound(stringBuf, feature_str, "|||");    // third field

    theScorer->prepareStats(sentence_index, sentence, scoreentry);
    scoredata->add(scoreentry, sentence_index);

    // examine first line for name of features
    if (!existsFeatureNames()) {
      InitFeatureMap(feature_str);
    }
    AddFeatures(feature_str, sentence_index);
  }
  inp.close();
}

void Data::InitFeatureMap(const string& str) {
  string buf = str;
  string substr;
  string features = "";
  string tmp_name = "";
  size_t tmp_index = 0;
  string::size_type loc;
  char tmp[64];                         // for snprintf();

  while (!buf.empty()) {
    getNextPound(buf, substr);

    // string ending with ":" are skipped, because they are the names of the features
    if ((loc = substr.find_last_of(":")) != substr.length()-1) {
      snprintf(tmp, sizeof(tmp), "%s_%lu ", tmp_name.c_str(), tmp_index);
      features.append(tmp);

      tmp_index++;
    } else if (substr.find("_") != string::npos) {
      // ignore sparse feature name and its value
      getNextPound(buf, substr);
    } else {                              // update current feature name
      tmp_index = 0;
      tmp_name = substr.substr(0, substr.size() - 1);
    }
  }
  featdata->setFeatureMap(features);
}

void Data::AddFeatures(const string& str,
                       const string& sentence_index) {
  string::size_type loc;
  string buf = str;
  string substr;
  FeatureStats feature_entry;
  feature_entry.reset();

  while (!buf.empty()) {
    getNextPound(buf, substr);

    // no ':' -> feature value that needs to be stored
    if ((loc = substr.find_last_of(":")) != substr.length()-1) {
      feature_entry.add(ConvertStringToFeatureStatsType(substr));
    } else if (substr.find("_") != string::npos) {
      // sparse feature name? store as well
      std::string name = substr;
      getNextPound(buf, substr);
      feature_entry.addSparse(name, atof(substr.c_str()));
      _sparse_flag = true;
    }
  }
  featdata->add(feature_entry, sentence_index);
}

// TODO
void Data::mergeSparseFeatures() {
  std::cerr << "ERROR: sparse features can only be trained with pairwise ranked optimizer (PRO), not traditional MERT\n";
  exit(1);
}

void Data::createShards(size_t shard_count, float shard_size, const string& scorerconfig,
                        std::vector<Data>& shards)
{
  CHECK(shard_count);
  CHECK(shard_size >= 0);
  CHECK(shard_size <= 1);

  size_t data_size = scoredata->size();
  CHECK(data_size == featdata->size());

  shard_size *= data_size;

  for (size_t shard_id = 0; shard_id < shard_count; ++shard_id) {
    vector<size_t> shard_contents;
    if (shard_size == 0) {
      //split into roughly equal size shards
      const size_t shard_start = floor(0.5 + shard_id * static_cast<float>(data_size) / shard_count);
      const size_t shard_end = floor(0.5 + (shard_id + 1) * static_cast<float>(data_size) / shard_count);
      for (size_t i = shard_start; i < shard_end; ++i) {
        shard_contents.push_back(i);
      }
    } else {
      //create shards by randomly sampling
      for (size_t i = 0; i < floor(shard_size+0.5); ++i) {
        shard_contents.push_back(rand() % data_size);
      }
    }

    Scorer* scorer = ScorerFactory::getScorer(score_type, scorerconfig);

    shards.push_back(Data(*scorer));
    shards.back().score_type = score_type;
    shards.back().number_of_scores = number_of_scores;
    shards.back()._sparse_flag = _sparse_flag;
    for (size_t i = 0; i < shard_contents.size(); ++i) {
      shards.back().featdata->add(featdata->get(shard_contents[i]));
      shards.back().scoredata->add(scoredata->get(shard_contents[i]));
    }
    //cerr << endl;
  }
}
