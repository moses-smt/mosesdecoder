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
  
  void FName::init(const string& name)  {
    Name2Id::iterator i = name2id.find(name);
    if (i != name2id.end()) {
      m_id = i->second;
    } else {
      m_id = name2id.size();
      name2id[name] = m_id;
      id2name.push_back(name);
    }
  }
  
  std::ostream& operator<<( std::ostream& out, const FName& name) {
    out << name.name();
    return out;
  }
  
  size_t FName::hash() const {
    /*std::size_t seed = 0;
		 boost::hash_combine(seed, m_root);
		 boost::hash_combine(seed, m_name);
		 return seed;*/
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
  
	FVector::FVector( FValue defaultValue)  
	{
    m_features[DEFAULT_NAME] = defaultValue;
	}
	
	void FVector::clear() {
    m_features.clear();
    m_features[DEFAULT_NAME] = DEFAULT;
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
  
  FName FVector::DEFAULT_NAME("DEFAULT","");
  const FValue FVector::DEFAULT = 0;
  
  
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
    if (get(DEFAULT_NAME) != rhs.get(DEFAULT_NAME)) return false;
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
	
	FValue FVector::operator[](const FName& name) const {
		return get(name) + get(DEFAULT_NAME);
	}
	
	
	
  ostream& FVector::print(ostream& out) const {
    out << "{";
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      FValue value = i->second;
      if (i->first != DEFAULT_NAME) {
        value += get(DEFAULT_NAME);
      }
      /*if (i->first != DEFAULT_NAME && i->second != 0.0) {
          out << i->first << "=" << value << ", ";
      }*/
      if (i->first != DEFAULT_NAME) {
    	  out << value << ", ";
      }
    }
    out << "}";
    return out;
  }
  
  ostream& operator<<(ostream& out, const FVector& fv) {
    return fv.print(out);
  }
	
  const FValue& FVector::get(const FName& name) const {
    const_iterator fi = m_features.find(name);
    if (fi == m_features.end()) {
      return DEFAULT;
    } else {
      return fi->second;
    }
  }

  float FVector::get(size_t index) const {
	  size_t pos = 0;
	  for (const_iterator i = cbegin(); i != cend(); ++i) {
		  FValue value = i->second;
		  if (pos == index) {
			  return value;
		  }
		  ++pos;
	  }
  }
	
  void FVector::set(const FName& name, const FValue& value) {
    m_features[name] = value;
  }
  
  void FVector::set(size_t index, float value) {
	  size_t pos = 0;
	  for (const_iterator i = cbegin(); i != cend(); ++i) {
		  if (pos == index) {
			  m_features[i->first] = value;
			  break;
		  }
		  ++pos;
	  }
  }

  void FVector::applyLog(size_t baseOfLog) {
	  for (const_iterator i = cbegin(); i != cend(); ++i) {
		  FValue value = i->second;
		  // log_a(value) = ln(value) / ln(a)
		  float logOfValue = 0;
		  if (value < 0) {
			  logOfValue = log(-1*value) / log(baseOfLog);
			  logOfValue *= -1;
		  }
		  else if (value > 0) {
			  logOfValue = log(value) / log(baseOfLog);
		  }
		  m_features[i->first] = logOfValue;
	  }
  }

  FVector& FVector::operator+= (const FVector& rhs) {
    //default value will take care of itself here.
    for (iterator i = begin(); i != end(); ++i) {
      set(i->first,i->second + rhs.get(i->first));
    }
    for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
      if (!hasNonDefaultValue(i->first)) {
        set(i->first,i->second);
      }
    }
    return *this;
  }
  
  FVector& FVector::operator-= (const FVector& rhs) {
    for (iterator i = begin(); i != end(); ++i) {
      set(i->first,i->second - rhs.get(i->first));
    }
    for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
      if (!hasNonDefaultValue(i->first)) {
        set(i->first,-(i->second));
      }
    }
    return *this;
  }
  
  FVector& FVector::operator*= (const FVector& rhs) {
    FValue lhsDefault = get(DEFAULT_NAME);
    FValue rhsDefault = rhs.get(DEFAULT_NAME);
    for (iterator i = begin(); i != end(); ++i) {
      if (i->first == DEFAULT_NAME) {
        set(i->first,lhsDefault*rhsDefault);
      } else {
        FValue lhsValue = i->second;
        FValue rhsValue = rhs.get(i->first);
        set(i->first, lhsValue*rhsDefault + rhsValue*lhsDefault + lhsValue*rhsValue);
      }
    }
    if (lhsDefault) {
      //Features that have the default value in the lhs
      for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
        if (!hasNonDefaultValue(i->first)) {
          set(i->first, lhsDefault*i->second);
        }
      }
    }
    return *this;
  }
  
  FVector& FVector::operator/= (const FVector& rhs) {
    FValue lhsDefault = get(DEFAULT_NAME);
    FValue rhsDefault = rhs.get(DEFAULT_NAME);
    if (lhsDefault && !rhsDefault) {
      throw runtime_error("Attempt to divide feature vectors where lhs has default and rhs does not");
    }
    FValue quotientDefault = 0;
    if (rhsDefault) {
      quotientDefault = lhsDefault / rhsDefault;
    }
    for (iterator i = begin(); i != end(); ++i) {
      if (i->first == DEFAULT_NAME) {
        set(i->first, quotientDefault);
      } else {
        FValue lhsValue = i->second;
        FValue rhsValue = rhs.get(i->first);
        set(i->first, (lhsValue + lhsDefault) / (rhsValue + rhsDefault) - quotientDefault);
      }
    }
    if (lhsDefault) {
      //Features that have the default value in the lhs
      for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
        if (!hasNonDefaultValue(i->first)) {
          set(i->first, lhsDefault / (i->second + rhsDefault) - quotientDefault);
        }
      }
    }
    return *this;
  }
  
	FVector& FVector::max_equals(const FVector& rhs) {
		FValue lhsDefault = get(DEFAULT_NAME);
		FValue rhsDefault = rhs.get(DEFAULT_NAME);
		FValue maxDefault = max(lhsDefault,rhsDefault);
		for (iterator i = begin(); i != end(); ++i) {
			if (i->first == DEFAULT_NAME) {
				set(i->first, maxDefault);
			} else {
				set(i->first, max(i->second + lhsDefault, rhs.get(i->first) + rhsDefault) - maxDefault);
			}
		}
		for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
			if (!hasNonDefaultValue(i->first)) {
				set(i->first, max(lhsDefault, (i->second + rhsDefault)) - maxDefault);
			}
		}
		
		
		return *this;
	}
  
  FVector& FVector::operator+= (const FValue& rhs) {
    set(DEFAULT_NAME, get(DEFAULT_NAME) + rhs);
    return *this;
  }
  
  FVector& FVector::operator-= (const FValue& rhs) {
    set(DEFAULT_NAME, get(DEFAULT_NAME) - rhs);
    return *this;
  }
  
  FVector& FVector::operator*= (const FValue& rhs) {
    //NB Could do this with boost::bind ?
    //This multiplies the default value, which is what we want
    for (iterator i = begin(); i != end(); ++i) {
      i->second *= rhs;
    }
    return *this;
  }
  
  
  FVector& FVector::operator/= (const FValue& rhs) {
		//This dividess the default value, which is what we want
    for (iterator i = begin(); i != end(); ++i) {
      i->second /= rhs;
    }
    return *this;
  }
  
  FValue FVector::l1norm() const {
    FValue norm = 0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      if ((i->second) && i->first == DEFAULT_NAME) {
        throw runtime_error("Cannot take l1norm with non-zero default values");
      }
      norm += abs(i->second);
    }
    return norm;
  }
  
  FValue FVector::sum() const {
    FValue sum = 0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      if ((i->second) && i->first == DEFAULT_NAME) {
        throw runtime_error("Cannot take sum with non-zero default values");
      }
      sum += i->second;
    }
    return sum;
  }
  
  FValue FVector::l2norm() const {
    return sqrt(inner_product(*this));
  }
  
	
  
  FValue FVector::inner_product(const FVector& rhs) const {
    FValue lhsDefault = get(DEFAULT_NAME);
    FValue rhsDefault = rhs.get(DEFAULT_NAME);
    if (lhsDefault && rhsDefault) {
      throw runtime_error("Cannot take inner product if both lhs and rhs have non-zero default values");
    }
    FValue product = 0.0;
    for (const_iterator i = cbegin(); i != cend(); ++i) {
      if (i->first != DEFAULT_NAME) {
        product += ((i->second + lhsDefault)*(rhs.get(i->first) + rhsDefault));
      }
    }
    
    if (lhsDefault) {
      //Features that have the default value in the rhs
      for (const_iterator i = rhs.cbegin(); i != rhs.cend(); ++i) {
        if (!hasNonDefaultValue(i->first)) {
          product += (i->second + rhsDefault)*lhsDefault;
        }
      }
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
  
  const FVector operator+(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) += rhs;
  }
  
  const FVector operator-(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) -= rhs;
  }
  
  const FVector operator*(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) *= rhs;
  }
  
  const FVector operator/(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) /= rhs;
  }
  
  const FVector fvmax(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs).max_equals(rhs);
  }
  
  FValue inner_product(const FVector& lhs, const FVector& rhs) {
    if (lhs.size() >= rhs.size()) {
      return rhs.inner_product(lhs);
    } else {
      return lhs.inner_product(rhs);
    }
  }
	
}
