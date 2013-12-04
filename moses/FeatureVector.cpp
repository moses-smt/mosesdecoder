/*
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 University of Edinburgh


 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "FeatureVector.h"
#include "util/string_piece_hash.hh"

using namespace std;


namespace Moses
{

const string FName::SEP = "_";
FName::Name2Id FName::name2id;
vector<string> FName::id2name;
FName::Id2Count FName::id2hopeCount;
FName::Id2Count FName::id2fearCount;
#ifdef WITH_THREADS
boost::shared_mutex FName::m_idLock;
#endif

void FName::init(const StringPiece &name)
{
#ifdef WITH_THREADS
  //reader lock
  boost::shared_lock<boost::shared_mutex> lock(m_idLock);
#endif
  Name2Id::iterator i = FindStringPiece(name2id, name);
  if (i != name2id.end()) {
    m_id = i->second;
  } else {
#ifdef WITH_THREADS
    //release the reader lock, and upgrade to writer lock
    lock.unlock();
    boost::unique_lock<boost::shared_mutex> write_lock(m_idLock);
#endif
    std::pair<std::string, size_t> to_ins;
    to_ins.first.assign(name.data(), name.size());
    to_ins.second = name2id.size();
    std::pair<Name2Id::iterator, bool> res(name2id.insert(to_ins));
    if (res.second) {
      // TODO this should be string pointers backed by the hash table.
      id2name.push_back(to_ins.first);
    }
    m_id = res.first->second;
  }
}

size_t FName::getId(const string& name)
{
  Name2Id::iterator i = name2id.find(name);
  assert (i != name2id.end());
  return i->second;
}

size_t FName::getHopeIdCount(const string& name)
{
  Name2Id::iterator i = name2id.find(name);
  if (i != name2id.end()) {
    float id = i->second;
    return id2hopeCount[id];
  }
  return 0;
}

size_t FName::getFearIdCount(const string& name)
{
  Name2Id::iterator i = name2id.find(name);
  if (i != name2id.end()) {
    float id = i->second;
    return id2fearCount[id];
  }
  return 0;
}

void FName::incrementHopeId(const string& name)
{
  Name2Id::iterator i = name2id.find(name);
  assert(i != name2id.end());
#ifdef WITH_THREADS
  // get upgradable lock and upgrade to writer lock
  boost::upgrade_lock<boost::shared_mutex> upgradeLock(m_idLock);
  boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
#endif
  id2hopeCount[i->second] += 1;
}

void FName::incrementFearId(const string& name)
{
  Name2Id::iterator i = name2id.find(name);
  assert(i != name2id.end());
#ifdef WITH_THREADS
  // get upgradable lock and upgrade to writer lock
  boost::upgrade_lock<boost::shared_mutex> upgradeLock(m_idLock);
  boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
#endif
  id2fearCount[i->second] += 1;
}

void FName::eraseId(size_t id)
{
#ifdef WITH_THREADS
  // get upgradable lock and upgrade to writer lock
  boost::upgrade_lock<boost::shared_mutex> upgradeLock(m_idLock);
  boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
#endif
  id2hopeCount.erase(id);
  id2fearCount.erase(id);
}

std::ostream& operator<<( std::ostream& out, const FName& name)
{
  out << name.name();
  return out;
}

size_t FName::hash() const
{
  return boost::hash_value(m_id);
}

const std::string& FName::name() const
{
  return id2name[m_id];
}


bool FName::operator==(const FName& rhs) const
{
  return m_id == rhs.m_id;
}

bool FName::operator!=(const FName& rhs) const
{
  return ! (*this == rhs);
}

FVector::FVector(size_t coreFeatures) : m_coreFeatures(coreFeatures) {}

void FVector::resize(size_t newsize)
{
  valarray<FValue> oldValues(m_coreFeatures);
  m_coreFeatures.resize(newsize);
  for (size_t i = 0; i < min(m_coreFeatures.size(), oldValues.size()); ++i) {
    m_coreFeatures[i] = oldValues[i];
  }
}

void FVector::clear()
{
  m_coreFeatures.resize(0);
  m_features.clear();
}

bool FVector::load(const std::string& filename)
{
  clear();
  ifstream in (filename.c_str());
  if (!in) {
    return false;
  }
  string line;
  while(getline(in,line)) {
    if (line[0] == '#') continue;
    istringstream linestream(line);
    string namestring;
    FValue value;
    linestream >> namestring;
    linestream >> value;
    FName fname(namestring);
    //cerr << "Setting sparse weight " << fname << " to value " << value << "." << endl;
    set(fname,value);
  }
  return true;
}

void FVector::save(const string& filename) const
{
  ofstream out(filename.c_str());
  if (!out) {
    ostringstream msg;
    msg << "Unable to open " << filename;
    throw runtime_error(msg.str());
  }
  write(out);
  out.close();
}

void FVector::write(ostream& out) const
{
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    out << i->first << " " << i->second << endl;
  }
}

static bool equalsTolerance(FValue lhs, FValue rhs)
{
  if (lhs == rhs) return true;
  static const FValue TOLERANCE = 1e-4;
  FValue diff = abs(lhs-rhs);
  FValue mean = (abs(lhs)+abs(rhs))/2;
  //cerr << "ET " << lhs << " " << rhs << " " << diff << " " << mean << " " << endl;
  return diff/mean < TOLERANCE ;
}

bool FVector::operator== (const FVector& rhs) const
{
  if (this == &rhs) {
    return true;
  }
  if (m_coreFeatures.size() != rhs.m_coreFeatures.size()) {
    return false;
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    if (!equalsTolerance(m_coreFeatures[i], rhs.m_coreFeatures[i])) return false;
  }
  for (const_iterator i  = cbegin(); i != cend(); ++i) {
    if (!equalsTolerance(i->second,rhs.get(i->first))) return false;
  }
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
    if (!equalsTolerance(i->second, get(i->first))) return false;
  }
  return true;
}

bool FVector::operator!= (const FVector& rhs) const
{
  return ! (*this == rhs);
}

ProxyFVector FVector::operator[](const FName& name)
{
  // At this point, we don't know whether operator[] was called, so we return
  // a proxy object and defer the decision until later
  return ProxyFVector(this, name);
}

/** Equivalent for core features. */
FValue& FVector::operator[](size_t index)
{
  return m_coreFeatures[index];
}


FValue FVector::operator[](const FName& name) const
{
  return get(name);
}

FValue FVector::operator[](size_t index) const
{
  return m_coreFeatures[index];
}

ostream& FVector::print(ostream& out) const
{
  out << "core=(";
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    out << m_coreFeatures[i];
    if (i + 1 < m_coreFeatures.size()) {
      out << ",";
    }
  }
  out << ") ";
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    if (i != cbegin())
      out << " ";
    out << i->first << "=" << i->second;
  }
  return out;
}

ostream& operator<<(ostream& out, const FVector& fv)
{
  return fv.print(out);
}

const FValue& FVector::get(const FName& name) const
{
  static const FValue DEFAULT = 0;
  const_iterator fi = m_features.find(name);
  if (fi == m_features.end()) {
    return DEFAULT;
  } else {
    return fi->second;
  }
}

FValue FVector::getBackoff(const FName& name, float backoff) const
{
  const_iterator fi = m_features.find(name);
  if (fi == m_features.end()) {
    return backoff;
  } else {
    return fi->second;
  }
}

void FVector::thresholdScale(FValue maxValue )
{
  FValue factor = 1.0;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    FValue value = i->second;
    if (abs(value)*factor > maxValue) {
      factor = abs(value) / maxValue;
    }
  }
  operator*=(factor);
}

void FVector::capMax(FValue maxValue)
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    if (i->second > maxValue)
      set(i->first, maxValue);
}

void FVector::capMin(FValue minValue)
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    if (i->second < minValue)
      set(i->first, minValue);
}

void FVector::set(const FName& name, const FValue& value)
{
  m_features[name] = value;
}

void FVector::printCoreFeatures()
{
  cerr << "core=(";
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    cerr << m_coreFeatures[i];
    if (i + 1 < m_coreFeatures.size()) {
      cerr << ",";
    }
  }
  cerr << ") ";
}

FVector& FVector::operator+= (const FVector& rhs)
{
  if (rhs.m_coreFeatures.size() > m_coreFeatures.size())
    resize(rhs.m_coreFeatures.size());
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    set(i->first, get(i->first) + i->second);
  for (size_t i = 0; i < rhs.m_coreFeatures.size(); ++i)
    m_coreFeatures[i] += rhs.m_coreFeatures[i];
  return *this;
}

// add only sparse features
void FVector::sparsePlusEquals(const FVector& rhs)
{
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    set(i->first, get(i->first) + i->second);
}

// assign only core features
void FVector::coreAssign(const FVector& rhs)
{
  for (size_t i = 0; i < rhs.m_coreFeatures.size(); ++i)
    m_coreFeatures[i] = rhs.m_coreFeatures[i];
}

void FVector::incrementSparseHopeFeatures()
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    FName::incrementHopeId((i->first).name());
}

void FVector::incrementSparseFearFeatures()
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    FName::incrementFearId((i->first).name());
}

void FVector::printSparseHopeFeatureCounts(std::ofstream& out)
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    out << (i->first).name() << ": " << FName::getHopeIdCount((i->first).name()) << std::endl;
}

void FVector::printSparseFearFeatureCounts(std::ofstream& out)
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    out << (i->first).name() << ": " << FName::getFearIdCount((i->first).name()) << std::endl;
}

void FVector::printSparseHopeFeatureCounts()
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    std::cerr << (i->first).name() << ": " << FName::getHopeIdCount((i->first).name()) << std::endl;
}

void FVector::printSparseFearFeatureCounts()
{
  for (const_iterator i = cbegin(); i != cend(); ++i)
    std::cerr << (i->first).name() << ": " << FName::getFearIdCount((i->first).name()) << std::endl;
}

size_t FVector::pruneSparseFeatures(size_t threshold)
{
  size_t count = 0;
  vector<FName> toErase;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    const std::string& fname = (i->first).name();
    if (FName::getHopeIdCount(fname) < threshold && FName::getFearIdCount(fname) < threshold) {
      toErase.push_back(i->first);
      std::cerr << "pruning: " << fname << " (" << FName::getHopeIdCount(fname) << ", " << FName::getFearIdCount(fname) << ")" << std::endl;
      FName::eraseId(FName::getId(fname));
      ++count;
    }
  }

  for (size_t i = 0; i < toErase.size(); ++i)
    m_features.erase(toErase[i]);

  return count;
}

size_t FVector::pruneZeroWeightFeatures()
{
  size_t count = 0;
  vector<FName> toErase;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    const std::string& fname = (i->first).name();
    if (i->second == 0) {
      toErase.push_back(i->first);
      //std::cerr << "prune: " << fname << std::endl;
      FName::eraseId(FName::getId(fname));
      ++count;
    }
  }

  for (size_t i = 0; i < toErase.size(); ++i)
    m_features.erase(toErase[i]);

  return count;
}

void FVector::updateConfidenceCounts(const FVector& weightUpdate, bool signedCounts)
{
  for (size_t i = 0; i < weightUpdate.m_coreFeatures.size(); ++i) {
    if (signedCounts) {
      //int sign = weightUpdate.m_coreFeatures[i] >= 0 ? 1 : -1;
      //m_coreFeatures[i] += (weightUpdate.m_coreFeatures[i] * weightUpdate.m_coreFeatures[i]) * sign;
      m_coreFeatures[i] += weightUpdate.m_coreFeatures[i];
    } else
      //m_coreFeatures[i] += (weightUpdate.m_coreFeatures[i] * weightUpdate.m_coreFeatures[i]);
      m_coreFeatures[i] += abs(weightUpdate.m_coreFeatures[i]);
  }

  for (const_iterator i = weightUpdate.cbegin(); i != weightUpdate.cend(); ++i) {
    if (weightUpdate[i->first] == 0)
      continue;
    float value = get(i->first);
    if (signedCounts) {
      //int sign = weightUpdate[i->first] >= 0 ? 1 : -1;
      //value += (weightUpdate[i->first] * weightUpdate[i->first]) * sign;
      value += weightUpdate[i->first];
    } else
      //value += (weightUpdate[i->first] * weightUpdate[i->first]);
      value += abs(weightUpdate[i->first]);
    set(i->first, value);
  }
}

void FVector::updateLearningRates(float decay_core, float decay_sparse, const FVector &confidenceCounts, float core_r0, float sparse_r0)
{
  for (size_t i = 0; i < confidenceCounts.m_coreFeatures.size(); ++i) {
    m_coreFeatures[i] = 1.0/(1.0/core_r0 + decay_core * abs(confidenceCounts.m_coreFeatures[i]));
  }

  for (const_iterator i = confidenceCounts.cbegin(); i != confidenceCounts.cend(); ++i) {
    float value = 1.0/(1.0/sparse_r0 + decay_sparse * abs(i->second));
    set(i->first, value);
  }
}

// count non-zero occurrences for all sparse features
void FVector::setToBinaryOf(const FVector& rhs)
{
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    if (rhs.get(i->first) != 0)
      set(i->first, 1);
  for (size_t i = 0; i < rhs.m_coreFeatures.size(); ++i)
    m_coreFeatures[i] = 1;
}

// divide only core features by scalar
FVector& FVector::coreDivideEquals(float scalar)
{
  for (size_t i = 0; i < m_coreFeatures.size(); ++i)
    m_coreFeatures[i] /= scalar;
  return *this;
}

// lhs vector is a sum of vectors, rhs vector holds number of non-zero summands
FVector& FVector::divideEquals(const FVector& rhs)
{
  assert(m_coreFeatures.size() == rhs.m_coreFeatures.size());
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    set(i->first, get(i->first)/rhs.get(i->first)); // divide by number of summands
  for (size_t i = 0; i < rhs.m_coreFeatures.size(); ++i)
    m_coreFeatures[i] /= rhs.m_coreFeatures[i]; // divide by number of summands
  return *this;
}

FVector& FVector::operator-= (const FVector& rhs)
{
  if (rhs.m_coreFeatures.size() > m_coreFeatures.size())
    resize(rhs.m_coreFeatures.size());
  for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    set(i->first, get(i->first) -(i->second));
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    if (i < rhs.m_coreFeatures.size()) {
      m_coreFeatures[i] -= rhs.m_coreFeatures[i];
    }
  }
  return *this;
}

FVector& FVector::operator*= (const FVector& rhs)
{
  if (rhs.m_coreFeatures.size() > m_coreFeatures.size()) {
    resize(rhs.m_coreFeatures.size());
  }
  for (iterator i = begin(); i != end(); ++i) {
    FValue lhsValue = i->second;
    FValue rhsValue = rhs.get(i->first);
    set(i->first,lhsValue*rhsValue);
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    if (i < rhs.m_coreFeatures.size()) {
      m_coreFeatures[i] *= rhs.m_coreFeatures[i];
    } else {
      m_coreFeatures[i] = 0;
    }
  }
  return *this;
}

FVector& FVector::operator/= (const FVector& rhs)
{
  if (rhs.m_coreFeatures.size() > m_coreFeatures.size()) {
    resize(rhs.m_coreFeatures.size());
  }
  for (iterator i = begin(); i != end(); ++i) {
    FValue lhsValue = i->second;
    FValue rhsValue = rhs.get(i->first);
    set(i->first, lhsValue / rhsValue) ;
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    if (i < rhs.m_coreFeatures.size()) {
      m_coreFeatures[i] /= rhs.m_coreFeatures[i];
    } else {
      if (m_coreFeatures[i] < 0) {
        m_coreFeatures[i] = -numeric_limits<FValue>::infinity();
      } else if (m_coreFeatures[i] > 0) {
        m_coreFeatures[i] = numeric_limits<FValue>::infinity();
      }
    }
  }
  return *this;
}

FVector& FVector::operator*= (const FValue& rhs)
{
  //NB Could do this with boost::bind ?
  for (iterator i = begin(); i != end(); ++i) {
    i->second *= rhs;
  }
  m_coreFeatures *= rhs;
  return *this;
}

FVector& FVector::operator/= (const FValue& rhs)
{
  for (iterator i = begin(); i != end(); ++i) {
    i->second /= rhs;
  }
  m_coreFeatures /= rhs;
  return *this;
}

FVector& FVector::multiplyEqualsBackoff(const FVector& rhs, float backoff)
{
  if (rhs.m_coreFeatures.size() > m_coreFeatures.size()) {
    resize(rhs.m_coreFeatures.size());
  }
  for (iterator i = begin(); i != end(); ++i) {
    FValue lhsValue = i->second;
    FValue rhsValue = rhs.getBackoff(i->first, backoff);
    set(i->first,lhsValue*rhsValue);
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    if (i < rhs.m_coreFeatures.size()) {
      m_coreFeatures[i] *= rhs.m_coreFeatures[i];
    } else {
      m_coreFeatures[i] = 0;
    }
  }
  return *this;
}

FVector& FVector::multiplyEquals(float core_r0, float sparse_r0)
{
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    m_coreFeatures[i] *= core_r0;
  }
  for (iterator i = begin(); i != end(); ++i)
    set(i->first,(i->second)*sparse_r0);
  return *this;
}

FValue FVector::l1norm() const
{
  FValue norm = 0;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    norm += abs(i->second);
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    norm += abs(m_coreFeatures[i]);
  }
  return norm;
}

FValue FVector::l1norm_coreFeatures() const
{
  FValue norm = 0;
  // ignore Bleu score feature (last feature)
  for (size_t i = 0; i < m_coreFeatures.size()-1; ++i)
    norm += abs(m_coreFeatures[i]);
  return norm;
}

FValue FVector::l2norm() const
{
  return sqrt(inner_product(*this));
}

FValue FVector::linfnorm() const
{
  FValue norm = 0;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    float absValue = abs(i->second);
    if (absValue > norm)
      norm = absValue;
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    float absValue = abs(m_coreFeatures[i]);
    if (absValue > norm)
      norm = absValue;
  }
  return norm;
}

size_t FVector::l1regularize(float lambda)
{
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    float value = m_coreFeatures[i];
    if (value > 0) {
      m_coreFeatures[i] = max(0.0f, value - lambda);
    } else {
      m_coreFeatures[i] = min(0.0f, value + lambda);
    }
  }

  size_t numberPruned = size();
  vector<FName> toErase;
  for (iterator i = begin(); i != end(); ++i) {
    float value = i->second;
    if (value != 0.0f) {
      if (value > 0)
        value = max(0.0f, value - lambda);
      else
        value = min(0.0f, value + lambda);

      if (value != 0.0f)
        i->second = value;
      else {
        toErase.push_back(i->first);
        const std::string& fname = (i->first).name();
        FName::eraseId(FName::getId(fname));
      }
    }
  }

  // erase features that have become zero
  for (size_t i = 0; i < toErase.size(); ++i)
    m_features.erase(toErase[i]);
  numberPruned -= size();
  return numberPruned;
}

void FVector::l2regularize(float lambda)
{
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    m_coreFeatures[i] *= (1 - lambda);
  }

  for (iterator i = begin(); i != end(); ++i) {
    i->second *= (1 - lambda);
  }
}

size_t FVector::sparseL1regularize(float lambda)
{
  /*for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    float value = m_coreFeatures[i];
    if (value > 0) {
      m_coreFeatures[i] = max(0.0f, value - lambda);
    }
    else {
      m_coreFeatures[i] = min(0.0f, value + lambda);
    }
    }*/

  size_t numberPruned = size();
  vector<FName> toErase;
  for (iterator i = begin(); i != end(); ++i) {
    float value = i->second;
    if (value != 0.0f) {
      if (value > 0)
        value = max(0.0f, value - lambda);
      else
        value = min(0.0f, value + lambda);

      if (value != 0.0f)
        i->second = value;
      else {
        toErase.push_back(i->first);
        const std::string& fname = (i->first).name();
        FName::eraseId(FName::getId(fname));
      }
    }
  }

  // erase features that have become zero
  for (size_t i = 0; i < toErase.size(); ++i)
    m_features.erase(toErase[i]);
  numberPruned -= size();
  return numberPruned;
}

void FVector::sparseL2regularize(float lambda)
{
  /*for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    m_coreFeatures[i] *= (1 - lambda);
    }*/

  for (iterator i = begin(); i != end(); ++i) {
    i->second *= (1 - lambda);
  }
}

FValue FVector::sum() const
{
  FValue sum = 0;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    sum += i->second;
  }
  sum += m_coreFeatures.sum();
  return sum;
}

FValue FVector::inner_product(const FVector& rhs) const
{
  assert(m_coreFeatures.size() == rhs.m_coreFeatures.size());
  FValue product = 0.0;
  for (const_iterator i = cbegin(); i != cend(); ++i) {
    product += ((i->second)*(rhs.get(i->first)));
  }
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    product += m_coreFeatures[i]*rhs.m_coreFeatures[i];
  }
  return product;
}

void FVector::merge(const FVector &other)
{
  // dense
  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
    FValue &thisVal = m_coreFeatures[i];
    const FValue otherVal = other.m_coreFeatures[i];

    if (otherVal) {
      assert(thisVal == 0 || thisVal == otherVal);
      thisVal = otherVal;
    }
  }

  // sparse
  FNVmap::const_iterator iter;
  for (iter = other.m_features.begin(); iter != other.m_features.end(); ++iter) {
    const FName  &otherKey = iter->first;
    const FValue otherVal = iter->second;
    m_features[otherKey] = otherVal;
  }
}

const FVector operator+(const FVector& lhs, const FVector& rhs)
{
  return FVector(lhs) += rhs;
}

const FVector operator-(const FVector& lhs, const FVector& rhs)
{
  return FVector(lhs) -= rhs;
}

const FVector operator*(const FVector& lhs, const FVector& rhs)
{
  return FVector(lhs) *= rhs;
}

const FVector operator/(const FVector& lhs, const FVector& rhs)
{
  return FVector(lhs) /= rhs;
}


const FVector operator*(const FVector& lhs, const FValue& rhs)
{
  return FVector(lhs) *= rhs;
}

const FVector operator/(const FVector& lhs, const FValue& rhs)
{
  return FVector(lhs) /= rhs;
}

FValue inner_product(const FVector& lhs, const FVector& rhs)
{
  if (lhs.size() >= rhs.size()) {
    return rhs.inner_product(lhs);
  } else {
    return lhs.inner_product(rhs);
  }
}
}
