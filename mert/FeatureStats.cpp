/*
 *  FeatureStats.cpp
 *  met - Minimum Error Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "FeatureStats.h"

#include <cmath>
#include <stdexcept>
#include "Util.h"

namespace {
const int kAvailableSize = 8;
} // namespace

SparseVector::name2id_t SparseVector::name2id_;
SparseVector::id2name_t SparseVector::id2name_;

FeatureStatsType SparseVector::get(const string& name) const {
  name2id_t::const_iterator name2id_iter = name2id_.find(name);
  if (name2id_iter == name2id_.end()) return 0;
  size_t id = name2id_iter->second;
  return get(id);
}

FeatureStatsType SparseVector::get(size_t id) const {
  fvector_t::const_iterator fvector_iter = fvector_.find(id);
  if (fvector_iter == fvector_.end()) return 0;
  return fvector_iter->second;
}

void SparseVector::set(const string& name, FeatureStatsType value) {
  name2id_t::const_iterator name2id_iter = name2id_.find(name);
  size_t id = 0;
  if (name2id_iter == name2id_.end()) {
    id = id2name_.size();
    id2name_.push_back(name);
    name2id_[name] = id;
  } else {
    id = name2id_iter->second;
  }
  fvector_[id] = value;
}

void SparseVector::write(ostream& out, const string& sep) const {
  for (fvector_t::const_iterator i = fvector_.begin(); i != fvector_.end(); ++i) {
    if (abs((float)(i->second)) < 0.00001) continue;
    string name = id2name_[i->first];
    out << name << sep << i->second << " ";
  }
}

void SparseVector::clear() {
  fvector_.clear();
}

void SparseVector::load(const string& file) {
  ifstream in(file.c_str());
  if (!in) {
    throw runtime_error("Failed to open sparse weights file: " + file);
  }
  string line;
  while(getline(in,line)) {
    if (line[0] == '#') continue;
    istringstream linestream(line);
    string name;
    float value;
    linestream >> name;
    linestream >> value;
    set(name,value);
  }
}

SparseVector& SparseVector::operator-=(const SparseVector& rhs) {

  for (fvector_t::const_iterator i = rhs.fvector_.begin();
      i != rhs.fvector_.end(); ++i) {
    fvector_[i->first] =  get(i->first) - (i->second);
  }
  return *this;
}

FeatureStatsType SparseVector::inner_product(const SparseVector& rhs) const {
  FeatureStatsType product = 0.0;
  for (fvector_t::const_iterator i = fvector_.begin();
    i != fvector_.end(); ++i) {
    product += ((i->second) * (rhs.get(i->first)));
  }
  return product;
}

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs) {
  SparseVector res(lhs);
  res -= rhs;
  return res;
}

FeatureStatsType inner_product(const SparseVector& lhs, const SparseVector& rhs) {
    if (lhs.size() >= rhs.size()) {
      return rhs.inner_product(lhs);
    } else {
      return lhs.inner_product(rhs);
    }
}

FeatureStats::FeatureStats()
    : available_(kAvailableSize), entries_(0),
      array_(new FeatureStatsType[available_]) {}

FeatureStats::FeatureStats(const size_t size)
    : available_(size), entries_(size),
      array_(new FeatureStatsType[available_])
{
  memset(array_, 0, GetArraySizeWithBytes());
}

FeatureStats::~FeatureStats()
{
  if (array_) {
    delete [] array_;
    array_ = NULL;
  }
}

void FeatureStats::Copy(const FeatureStats &stats)
{
  available_ = stats.available();
  entries_ = stats.size();
  array_ = new FeatureStatsType[available_];
  memcpy(array_, stats.getArray(), GetArraySizeWithBytes());
  map_ = stats.getSparse();
}

FeatureStats::FeatureStats(const FeatureStats &stats)
{
  Copy(stats);
}

FeatureStats& FeatureStats::operator=(const FeatureStats &stats)
{
  delete [] array_;
  Copy(stats);
  return *this;
}

void FeatureStats::expand()
{
  available_ *= 2;
  featstats_t t_ = new FeatureStatsType[available_];
  memcpy(t_, array_, GetArraySizeWithBytes());
  delete [] array_;
  array_ = t_;
}

void FeatureStats::add(FeatureStatsType v)
{
  if (isfull()) expand();
  array_[entries_++]=v;
}

void FeatureStats::addSparse(const string& name, FeatureStatsType v)
{
  map_.set(name,v);
}

void FeatureStats::set(std::string &theString, const SparseVector& sparseWeights)
{
  std::string substring, stringBuf;
  reset();

  while (!theString.empty()) {
    getNextPound(theString, substring);
    // regular feature
    if (substring.find(":") == string::npos) {
      add(ConvertStringToFeatureStatsType(substring));
    }
    // sparse feature
    else {
      size_t separator = substring.find_last_of(":");
      addSparse(substring.substr(0,separator), atof(substring.substr(separator+1).c_str()) );
    }
  }

  if (sparseWeights.size()) {
    //Merge the sparse features
    FeatureStatsType merged = inner_product(sparseWeights, map_);
    add(merged);
    /*
    cerr << "Merged ";
    sparseWeights.write(cerr,"=");
    cerr << " and ";
    map_.write(cerr,"=");
    cerr << " to give " <<  merged << endl;
    */
    map_.clear();
  }
  /*
  cerr << "FS: ";
  for (size_t i = 0; i < entries_; ++i) {
    cerr << array_[i] << " ";
  }
  cerr << endl;*/
}


void FeatureStats::loadbin(std::ifstream& inFile)
{
  inFile.read((char*) array_, GetArraySizeWithBytes());
}

void FeatureStats::loadtxt(std::ifstream& inFile, const SparseVector& sparseWeights)
{
  std::string theString;
  std::getline(inFile, theString);
  set(theString, sparseWeights);
}


void FeatureStats::savetxt(const std::string &file)
{
//      TRACE_ERR("saving the stats into " << file << std::endl);

  std::ofstream outFile(file.c_str(), std::ios::out); // matches a stream with a file. Opens the file

  savetxt(outFile);
}


void FeatureStats::savetxt(std::ofstream& outFile)
{
//      TRACE_ERR("saving the stats" << std::endl);
  outFile << *this;
}

void FeatureStats::savebin(std::ofstream& outFile)
{
  outFile.write((char*) array_, GetArraySizeWithBytes());
}

ostream& operator<<(ostream& o, const FeatureStats& e)
{
  // print regular features
  for (size_t i=0; i< e.size(); i++) {
    o << e.get(i) << " ";
  }
  // sparse features
  e.getSparse().write(o,"");

  return o;
}
