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
#include <cassert>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "FeatureVector.h"

using namespace std;


namespace Moses {
  
  const string FName::SEP = "_";
  FName::Name2Id FName::name2id;
  vector<string> FName::id2name;
#ifdef WITH_THREADS
  boost::shared_mutex FName::m_idLock;
#endif
  
  void FName::init(const string& name)  {
#ifdef WITH_THREADS
    //reader lock
    boost::shared_lock<boost::shared_mutex> lock(m_idLock);
#endif
    Name2Id::iterator i = name2id.find(name);
    if (i != name2id.end()) {
      m_id = i->second;
    } else {
#ifdef WITH_THREADS
      //release the reader lock, and upgrade to writer lock
      lock.unlock();
      boost::upgrade_lock<boost::shared_mutex> upgradeLock(m_idLock);
      boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(upgradeLock);
#endif
      //Need to check again if the id is in the map, as someone may have added
      //it while we were waiting on the writer lock.
      if (i != name2id.end()) {
        m_id = i->second;
      } else {
        m_id = name2id.size();
        name2id[name] = m_id;
        id2name.push_back(name);
      }
    }
  }
  
  std::ostream& operator<<( std::ostream& out, const FName& name) {
    out << name.name();
    return out;
  }
  
  size_t FName::hash() const {
    return boost::hash_value(m_id);
  }
  
  const std::string& FName::name() const {
    return id2name[m_id];
  }
  
  
  bool FName::operator==(const FName& rhs) const {
    return m_id == rhs.m_id;
  }
  
  bool FName::operator!=(const FName& rhs) const {
    return ! (*this == rhs);
  }
  
  FVector::FVector(size_t coreFeatures) : m_coreFeatures(coreFeatures) {}

  void FVector::resize(size_t newsize) {
      valarray<FValue> oldValues(m_coreFeatures);
      m_coreFeatures.resize(newsize);
      for (size_t i = 0; i < min(m_coreFeatures.size(), oldValues.size()); ++i) {
        m_coreFeatures[i] = oldValues[i];
      }
    }
	
	void FVector::clear() {
	  m_coreFeatures.resize(0);
	  m_features.clear();
	}
	
	bool FVector::load(const std::string& filename) {
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
			set(fname,value);
		}
    return true;
  }

  void FVector::save(const string& filename) const {
    ofstream out(filename.c_str());
    if (!out) {
      ostringstream msg;
      msg << "Unable to open " << filename;
      throw runtime_error(msg.str());
    }
    write(out);
    out.close();
  }

  void FVector::write(ostream& out) const {
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      out << i->first << " " << i->second << endl;
    }
  }

  static bool equalsTolerance(FValue lhs, FValue rhs) {
    if (lhs == rhs) return true;
    static const FValue TOLERANCE = 1e-4;
    FValue diff = abs(lhs-rhs);
    FValue mean = (abs(lhs)+abs(rhs))/2;
    //cerr << "ET " << lhs << " " << rhs << " " << diff << " " << mean << " " << endl;
    return diff/mean < TOLERANCE ;
  }
  
  bool FVector::operator== (const FVector& rhs) const {
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
  
  bool FVector::operator!= (const FVector& rhs) const {
    return ! (*this == rhs);
  }
	
  ProxyFVector FVector::operator[](const FName& name) {
	  // At this point, we don't know whether operator[] was called, so we return
	  // a proxy object and defer the decision until later
	  return ProxyFVector(this, name);
  }

  /** Equivalent for core features. */
  FValue& FVector::operator[](size_t index) {
    return m_coreFeatures[index];
  }

	
  FValue FVector::operator[](const FName& name) const {
	  return get(name);
  }

  FValue FVector::operator[](size_t index) const {
    return m_coreFeatures[index];
  }

  ostream& FVector::print(ostream& out) const {
	out << "{";
    out << "core=(";
    for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
      out << m_coreFeatures[i];
      if (i + 1 < m_coreFeatures.size()) {
        out << ",";
      }
    }
    out << ") ";
    for (const_iterator i = cbegin(); i != cend(); ++i) {
       out << i->first << "=" << i->second << ", ";
    }
    out << "}";
    return out;
  }
  
  ostream& operator<<(ostream& out, const FVector& fv) {
    return fv.print(out);
  }
	
  const FValue& FVector::get(const FName& name) const {
    static const FValue DEFAULT = 0;
    const_iterator fi = m_features.find(name);
    if (fi == m_features.end()) {
      return DEFAULT;
    } else {
      return fi->second;
    }
  }

  void FVector::thresholdScale(FValue maxValue ) {
    FValue factor = 1.0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      FValue value = i->second;
      if (abs(value)*factor > maxValue) {
        factor = abs(value) / maxValue;
      }
    }
    operator*=(factor);
  }

  void FVector::set(const FName& name, const FValue& value) {
    m_features[name] = value;
  }
  
  void FVector::logCoreFeatures(size_t baseOfLog) {
  	float logOfValue = 0;
  	// log_a(value) = ln(value) / ln(a)
	  for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
		  FValue value = m_coreFeatures[i];
		  if (value == 0) continue;
		  else if (value < 0) {
			  logOfValue = log(-1*value) / log(baseOfLog);
			  logOfValue *= -1;
		  }
		  else
		  	logOfValue = log(value) / log(baseOfLog);
		  m_coreFeatures[i] = logOfValue;
	  }
  }

  FVector& FVector::operator+= (const FVector& rhs) {
    if (rhs.m_coreFeatures.size() > m_coreFeatures.size())
      resize(rhs.m_coreFeatures.size());
    for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i)
    	set(i->first, get(i->first) + i->second);
    for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
      if (i < rhs.m_coreFeatures.size()) {
        m_coreFeatures[i] += rhs.m_coreFeatures[i];
      }
    }
    return *this;
  }
  
  FVector& FVector::operator-= (const FVector& rhs) {
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
  
  FVector& FVector::operator*= (const FVector& rhs) {
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
  
  FVector& FVector::operator/= (const FVector& rhs) {
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
  
  FVector& FVector::operator*= (const FValue& rhs) {
    //NB Could do this with boost::bind ?
    for (iterator i = begin(); i != end(); ++i) {
      i->second *= rhs;
    }
    m_coreFeatures *= rhs;
    return *this;
  }
  
  FVector& FVector::operator/= (const FValue& rhs) {
    for (iterator i = begin(); i != end(); ++i) {
      i->second /= rhs;
    }
    m_coreFeatures /= rhs;
    return *this;
  }
  
  FValue FVector::l1norm() const {
    FValue norm = 0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      norm += abs(i->second);
    }
    for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
      norm += abs(m_coreFeatures[i]);
    }
    return norm;
  }
  
  FValue FVector::l2norm() const {
    return sqrt(inner_product(*this));
  }

  FValue FVector::linfnorm() const {
    FValue norm = 0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      float absValue = abs(i->second);
      if (absValue > norm)
	norm = absValue;
    }
    for (size_t i = 0; i < m_coreFeatures.size(); ++i) {
      float absValue = m_coreFeatures[i];
      if (absValue > norm)
	norm = absValue;
    }
    return norm;
  }

  FValue FVector::sum() const {
    FValue sum = 0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      sum += i->second;
    }
    sum += m_coreFeatures.sum();
    return sum;
  }
    
  FValue FVector::inner_product(const FVector& rhs) const {
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

  const FVector operator+(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) += rhs;
  }
  
  const FVector operator-(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) -= rhs;
  }
  
	const FVector operator*(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) *= rhs;
  }
  
  const FVector operator/(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) /= rhs;
  }
  
  
  const FVector operator*(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) *= rhs;
  }
  
  const FVector operator/(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) /= rhs;
  }

  FValue inner_product(const FVector& lhs, const FVector& rhs) {
    if (lhs.size() >= rhs.size()) {
      return rhs.inner_product(lhs);
    } else {
      return lhs.inner_product(rhs);
    }
  }
	
}
