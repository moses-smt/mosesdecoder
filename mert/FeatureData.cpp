/*
 *  FeatureData.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "FeatureData.h"

#include <limits>
#include "FileStream.h"
#include "Util.h"
#include <cstdio>

static const float MIN_FLOAT=-1.0*numeric_limits<float>::max();
static const float MAX_FLOAT=numeric_limits<float>::max();

FeatureData::FeatureData()
    : number_of_features(0),
      _sparse_flag(false) {}

void FeatureData::save(std::ofstream& outFile, bool bin)
{
  for (featdata_t::iterator i = array_.begin(); i !=array_.end(); i++)
    i->save(outFile, bin);
}

void FeatureData::save(const std::string &file, bool bin)
{
  if (file.empty()) return;

  TRACE_ERR("saving the array into " << file << std::endl);

  std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

  save(outFile, bin);

  outFile.close();
}

void FeatureData::load(ifstream& inFile)
{
  FeatureArray entry;

  while (!inFile.eof()) {

    if (!inFile.good()) {
      std::cerr << "ERROR FeatureData::load inFile.good()" << std::endl;
    }

    entry.clear();
    entry.load(inFile);

    if (entry.size() == 0)
      break;

    if (size() == 0)
      setFeatureMap(entry.Features());

    if (entry.hasSparseFeatures())
      _sparse_flag = true;

    add(entry);
  }
}


void FeatureData::load(const std::string &file)
{
  TRACE_ERR("loading feature data from " << file << std::endl);

  inputfilestream inFile(file); // matches a stream with a file. Opens the file

  if (!inFile) {
    throw runtime_error("Unable to open feature file: " + file);
  }

  load((ifstream&) inFile);

  inFile.close();
}

void FeatureData::add(FeatureArray& e)
{
  if (exists(e.getIndex())) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(e.getIndex());
    array_.at(pos).merge(e);
  } else {
    array_.push_back(e);
    setIndex();
  }
}

void FeatureData::add(FeatureStats& e, const std::string& sent_idx)
{
  if (exists(sent_idx)) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(sent_idx);
//              TRACE_ERR("Inserting " << e << " in array " << sent_idx << std::endl);
    array_.at(pos).add(e);
  } else {
//              TRACE_ERR("Creating a new entry in the array and inserting " << e << std::endl);
    FeatureArray a;
    a.NumberOfFeatures(number_of_features);
    a.Features(features);
    a.setIndex(sent_idx);
    a.add(e);
    add(a);
  }
}

bool FeatureData::check_consistency() const
{
  if (array_.size() == 0)
    return true;

  for (featdata_t::const_iterator i = array_.begin(); i != array_.end(); i++)
    if (!i->check_consistency()) return false;

  return true;
}

void FeatureData::setIndex()
{
  size_t j=0;
  for (featdata_t::iterator i = array_.begin(); i !=array_.end(); i++) {
    idx2arrayname_[j]=(*i).getIndex();
    arrayname2idx_[(*i).getIndex()] = j;
    j++;
  }
}

void FeatureData::setFeatureMap(const std::string& feat)
{
  number_of_features = 0;
  features = feat;

  std::string substring, stringBuf;
  stringBuf = features;
  while (!stringBuf.empty()) {
    getNextPound(stringBuf, substring);

    featname2idx_[substring] = idx2featname_.size();
    idx2featname_[idx2featname_.size()] = substring;
    number_of_features++;
  }
}

string FeatureData::ToString() const {
  string res;
  char buf[100];

  snprintf(buf, sizeof(buf), "number of features: %lu, ", number_of_features);
  res.append(buf);

  snprintf(buf, sizeof(buf), "features: ");
  res.append(buf);
  res.append(features);

  snprintf(buf, sizeof(buf), ", sparse flag: %s, ", (_sparse_flag) ? "yes" : "no");
  res.append(buf);

  snprintf(buf, sizeof(buf), "feature_id_map = { ");
  res.append(buf);
  for (map<string, size_t>::const_iterator it = featname2idx_.begin();
       it != featname2idx_.end(); ++it) {
    snprintf(buf, sizeof(buf), "%s => %lu, ",
                  it->first.c_str(), it->second);
    res.append(buf);
  }
  snprintf(buf, sizeof(buf), "}");
  res.append(buf);

  return res;
}
