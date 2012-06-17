/*
 *  FeatureStats.cpp
 *  mert - Minimum Error Rate Training
 *
 *  Created by Nicola Bertoldi on 13/05/08.
 *
 */

#include "FeatureStats.h"

#include <fstream>
#include <cmath>
#include <boost/functional/hash.hpp>

#include "Util.h"

using namespace std;

namespace {
const int kAvailableSize = 8;
} // namespace

SparseVector::name2id_t SparseVector::m_name_to_id;
SparseVector::id2name_t SparseVector::m_id_to_name;

FeatureStatsType SparseVector::get(const string& name) const {
  name2id_t::const_iterator name2id_iter = m_name_to_id.find(name);
  if (name2id_iter == m_name_to_id.end()) return 0;
  size_t id = name2id_iter->second;
  return get(id);
}

FeatureStatsType SparseVector::get(size_t id) const {
  fvector_t::const_iterator fvector_iter = m_fvector.find(id);
  if (fvector_iter == m_fvector.end()) return 0;
  return fvector_iter->second;
}

void SparseVector::set(const string& name, FeatureStatsType value) {
  name2id_t::const_iterator name2id_iter = m_name_to_id.find(name);
  size_t id = 0;
  if (name2id_iter == m_name_to_id.end()) {
    id = m_id_to_name.size();
    m_id_to_name.push_back(name);
    m_name_to_id[name] = id;
  } else {
    id = name2id_iter->second;
  }
  m_fvector[id] = value;
}

void SparseVector::write(ostream& out, const string& sep) const {
  for (fvector_t::const_iterator i = m_fvector.begin(); i != m_fvector.end(); ++i) {
    if (abs(i->second) < 0.00001) continue;
    string name = m_id_to_name[i->first];
    out << name << sep << i->second << " ";
  }
}

void SparseVector::clear() {
  m_fvector.clear();
}

SparseVector& SparseVector::operator-=(const SparseVector& rhs) {
  //All the elements that have values in *this
  for (fvector_t::iterator i = m_fvector.begin(); i != m_fvector.end(); ++i) {
    m_fvector[i->first] = i->second - rhs.get(i->first);
  }

  //Any elements in rhs, that have no value in *this
  for (fvector_t::const_iterator i = rhs.m_fvector.begin();
      i != rhs.m_fvector.end(); ++i) {
    if (m_fvector.find(i->first) == m_fvector.end()) {
      m_fvector[i->first] = -(i->second);
    }
  }
  return *this;
}

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs) {
  SparseVector res(lhs);
  res -= rhs;
  return res;
}

std::vector<std::size_t> SparseVector::feats() const {
  std::vector<std::size_t> toRet;
  for(fvector_t::const_iterator iter = m_fvector.begin();
      iter!=m_fvector.end();
      iter++) {
    toRet.push_back(iter->first);
  }
  return toRet;
}

std::size_t SparseVector::encode(const std::string& name) {
  name2id_t::const_iterator name2id_iter = m_name_to_id.find(name);
  size_t id = 0;
  if (name2id_iter == m_name_to_id.end()) {
    id = m_id_to_name.size();
    m_id_to_name.push_back(name);
    m_name_to_id[name] = id;
  } else {
    id = name2id_iter->second;
  }
  return id;
}

std::string SparseVector::decode(std::size_t id) {
  return m_id_to_name[id];
}

bool operator==(SparseVector const& item1, SparseVector const& item2) {
  return item1.m_fvector==item2.m_fvector;
}

std::size_t hash_value(SparseVector const& item) {
  boost::hash<SparseVector::fvector_t> hasher;
  return hasher(item.m_fvector);
}


FeatureStats::FeatureStats()
    : m_available_size(kAvailableSize), m_entries(0),
      m_array(new FeatureStatsType[m_available_size]) {}

FeatureStats::FeatureStats(const size_t size)
    : m_available_size(size), m_entries(size),
      m_array(new FeatureStatsType[m_available_size])
{
  memset(m_array, 0, GetArraySizeWithBytes());
}

FeatureStats::FeatureStats(string &theString)
    : m_available_size(0), m_entries(0), m_array(NULL)
{
  set(theString);
}

FeatureStats::~FeatureStats()
{
  if (m_array) {
    delete [] m_array;
    m_array = NULL;
  }
}

void FeatureStats::Copy(const FeatureStats &stats)
{
  m_available_size = stats.available();
  m_entries = stats.size();
  m_array = new FeatureStatsType[m_available_size];
  memcpy(m_array, stats.getArray(), GetArraySizeWithBytes());
  m_map = stats.getSparse();
}

FeatureStats::FeatureStats(const FeatureStats &stats)
{
  Copy(stats);
}

FeatureStats& FeatureStats::operator=(const FeatureStats &stats)
{
  delete [] m_array;
  Copy(stats);
  return *this;
}

void FeatureStats::expand()
{
  m_available_size *= 2;
  featstats_t t_ = new FeatureStatsType[m_available_size];
  memcpy(t_, m_array, GetArraySizeWithBytes());
  delete [] m_array;
  m_array = t_;
}

void FeatureStats::add(FeatureStatsType v)
{
  if (isfull()) expand();
  m_array[m_entries++]=v;
}

void FeatureStats::addSparse(const string& name, FeatureStatsType v)
{
  m_map.set(name,v);
}

void FeatureStats::set(string &theString)
{
  string substring, stringBuf;
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
}

void FeatureStats::loadbin(istream* is)
{
  is->read(reinterpret_cast<char*>(m_array),
           static_cast<streamsize>(GetArraySizeWithBytes()));
}

void FeatureStats::loadtxt(istream* is)
{
  string line;
  getline(*is, line);
  set(line);
}

void FeatureStats::loadtxt(const string &file)
{
  ifstream ifs(file.c_str(), ios::in);
  if (!ifs) {
    cerr << "Failed to open " << file << endl;
    exit(1);
  }
  istream* is = &ifs;
  loadtxt(is);
}

void FeatureStats::savetxt(const string &file)
{
  ofstream ofs(file.c_str(), ios::out);
  ostream* os = &ofs;
  savetxt(os);
}

void FeatureStats::savetxt(ostream* os)
{
  *os << *this;
}

void FeatureStats::savetxt() {
  savetxt(&cout);
}

void FeatureStats::savebin(ostream* os)
{
  os->write(reinterpret_cast<char*>(m_array),
            static_cast<streamsize>(GetArraySizeWithBytes()));
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

//ADEED_BY_TS
bool operator==(const FeatureStats& f1, const FeatureStats& f2) {
  size_t size = f1.size();

  if (size != f2.size())
    return false;

  for (size_t k=0; k < size; k++) {
    if (f1.get(k) != f2.get(k))
      return false;
  }

  return true;
}
//END_ADDED
