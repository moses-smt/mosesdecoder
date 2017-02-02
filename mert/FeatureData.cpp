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

using namespace std;

namespace MosesTuning
{



FeatureData::FeatureData()
  : m_num_features(0) {}

void FeatureData::save(ostream* os, bool bin)
{
  for (featdata_t::iterator i = m_array.begin(); i != m_array.end(); i++)
    i->save(os, bin);
}

void FeatureData::save(const string &file, bool bin)
{
  if (file.empty()) return;
  TRACE_ERR("saving the array into " << file << endl);
  ofstream ofs(file.c_str(), ios::out); // matches a stream with a file. Opens the file
  ostream* os = &ofs;
  save(os, bin);
  ofs.close();
}

void FeatureData::save(bool bin)
{
  save(&cout, bin);
}

void FeatureData::load(istream* is, const SparseVector& sparseWeights)
{
  FeatureArray entry;

  while (!is->eof()) {

    if (!is->good()) {
      cerr << "ERROR FeatureData::load inFile.good()" << endl;
    }

    entry.clear();
    entry.load(is, sparseWeights);

    if (entry.size() == 0)
      break;

    if (size() == 0)
      setFeatureMap(entry.Features());

    add(entry);
  }
}


void FeatureData::load(const string &file, const SparseVector& sparseWeights)
{
  TRACE_ERR("loading feature data from " << file << endl);
  inputfilestream input_stream(file); // matches a stream with a file. Opens the file
  if (!input_stream) {
    throw runtime_error("Unable to open feature file: " + file);
  }
  istream* is = &input_stream;
  load(is, sparseWeights);
  input_stream.close();
}

void FeatureData::add(FeatureArray& e)
{
  if (exists(e.getIndex())) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(e.getIndex());
    m_array.at(pos).merge(e);
  } else {
    m_array.push_back(e);
    setIndex();
  }
}

void FeatureData::add(FeatureStats& e, int sent_idx)
{
  if (exists(sent_idx)) { // array at position e.getIndex() already exists
    //enlarge array at position e.getIndex()
    size_t pos = getIndex(sent_idx);
//              TRACE_ERR("Inserting " << e << " in array " << sent_idx << std::endl);
    m_array.at(pos).add(e);
  } else {
//              TRACE_ERR("Creating a new entry in the array and inserting " << e << std::endl);
    FeatureArray a;
    a.NumberOfFeatures(m_num_features);
    a.Features(m_features);
    a.setIndex(sent_idx);
    a.add(e);
    add(a);
  }
}

bool FeatureData::check_consistency() const
{
  if (m_array.size() == 0)
    return true;

  for (featdata_t::const_iterator i = m_array.begin(); i != m_array.end(); i++)
    if (!i->check_consistency()) return false;

  return true;
}

void FeatureData::setIndex()
{
  size_t j=0;
  for (featdata_t::iterator i = m_array.begin(); i !=m_array.end(); i++) {
    m_index_to_array_name[j]=(*i).getIndex();
    m_array_name_to_index[(*i).getIndex()] = j;
    j++;
  }
}

void FeatureData::setFeatureMap(const string& feat)
{
  m_num_features = 0;
  m_features = feat;

  vector<string> buf;
  Tokenize(feat.c_str(), ' ', &buf);
  for (vector<string>::const_iterator it = buf.begin();
       it != buf.end(); ++it) {
    const size_t size = m_index_to_feature_name.size();
    m_feature_name_to_index[*it] = size;
    m_index_to_feature_name[size] = *it;
    ++m_num_features;
  }
}

string FeatureData::ToString() const
{
  string res;

  {
    stringstream ss;
    ss << "number of features: " << m_num_features
       << ", features: " << m_features;
    res.append(ss.str());
  }

  res.append("feature_id_map = { ");
  for (map<string, size_t>::const_iterator it = m_feature_name_to_index.begin();
       it != m_feature_name_to_index.end(); ++it) {
    stringstream ss;
    ss << it->first << " => " << it->second << ", ";
    res.append(ss.str());
  }
  res.append("}");

  return res;
}

}
