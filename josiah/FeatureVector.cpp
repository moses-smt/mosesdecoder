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
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "FeatureVector.h"

using namespace std;


namespace Josiah {
  
  static const string SEP = "_";
  
  FName::FName(const std::string& namestring) {
    size_t sep = namestring.find(SEP);
    if (sep != string::npos) {
      root = namestring.substr(0,sep);
      name = namestring.substr(sep+1);
    } else {
      root = namestring;
      name = "";
    }
  }
  
  std::ostream& operator<<( std::ostream& out, const FName& name) {
    out << name.root << SEP << name.name;
    return out;
  }
  
  bool FName::operator==(const FName& rhs) const {
    return root == rhs.root && name == rhs.name;
  }
  
  bool FName::operator!=(const FName& rhs) const {
    return ! (*this == rhs);
  }
  
   FVector::FVector(FValue defaultValue)  {
    m_features[DEFAULT_NAME] = defaultValue;
   }
   
   void FVector::clear() {
    m_features.clear();
    m_features[DEFAULT_NAME] = DEFAULT;
   }
   
   void FVector::load(const std::string& filename) {
     ifstream in (filename.c_str());
     if (!in) {
       ostringstream msg;
       msg << "Unable to open " << filename;
       throw runtime_error(msg.str());
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
    for (const_iterator i = begin(); i != end(); ++i) {
      out << i->first << " " << i->second << endl;
    }
  }
   
   FName FVector::DEFAULT_NAME("DEFAULT","");
   const FValue FVector::DEFAULT = 0;
   
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
    for (const_iterator i = begin(); i != end(); ++i) {
      FValue value = i->second;
      if (i->first != DEFAULT_NAME) {
        value += get(DEFAULT_NAME);
      }
      if (i->first != DEFAULT_NAME || i->second != 0.0) {
        out << i->first << "=" << value << ", ";
      }
    }
    out << "}";
    return out;
  }
  
  ostream& operator<<(ostream& out, const FVector& fv) {
    return fv.print(out);
  }
   
  const FValue& FVector::get(const FName& name) const {
    map<FName,FValue>::const_iterator fi = m_features.find(name);
    if (fi == m_features.end()) {
      return DEFAULT;
    } else {
      return fi->second;
    }
  }
   
  void FVector::set(const FName& name, const FValue& value) {
    m_features[name] = value;
  }
  
  FVector& FVector::operator+= (const FVector& rhs) {
    //default value will take care of itself here.
    for (iterator i = begin(); i != end(); ++i) {
      set(i->first,i->second + rhs.get(i->first));
    }
    for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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
    for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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
      for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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
      for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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
      for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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
  

  
  FValue FVector::inner_product(const FVector& rhs) const {
    FValue lhsDefault = get(DEFAULT_NAME);
    FValue rhsDefault = rhs.get(DEFAULT_NAME);
    if (lhsDefault && rhsDefault) {
      throw runtime_error("Cannot take inner product if both lhs and rhs have default values");
    }
    FValue product = 0.0;
    for (const_iterator i = begin(); i != end(); ++i) {
      if (i->first != DEFAULT_NAME) {
        product += (i->second + lhsDefault)*(rhs.get(i->first) + rhsDefault);
      }
    }
    
    if (lhsDefault) {
      //Features that have the default value in the rhs
      for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
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

