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

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "FeatureVector.h"

using namespace std;


namespace Josiah {
  
  static const string SEP = "_";
  
  std::ostream& operator<<( std::ostream& out, const FName& name) {
    out << name.root << SEP << name.name;
    return out;
  }
  
   FVector::FVector() {}
   
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
       size_t sep = namestring.find(SEP);
       string root,name;
       if (sep != string::npos) {
         root = namestring.substr(0,sep);
         name = namestring.substr(sep+1);
       } else {
         root = namestring;
       }
       FName fname(root,name);
       cerr << fname << " " << value << endl;
       set(fname,value);
     }
  }
   
   FValue FVector::DEFAULT = 0.0;
   
   ProxyFVector FVector::operator[](const FName& name) {
      // At this point, we don't know whether operator[] was called, so we return
      // a proxy object and defer the decision until later
      return ProxyFVector(this, name);

   }
   
   const FValue& FVector::operator[](const FName& name) const {
     return get(name);
   }
   
   bool FVector::hasValue(const FName& name) const {
    return m_features.find(name) != m_features.end();
   }
   
  ostream& FVector::print(ostream& out) const {
    out << "{";  
    for (map<FName,FValue>::const_iterator i = m_features.begin(); i != m_features.end(); ++i) {
      out << i->first << ":" << i->second << ",";
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
    for (iterator i = begin(); i != end(); ++i) {
      set(i->first,i->second + rhs[i->first]);
    }
    for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
      if (!hasValue(i->first)) {
        set(i->first,i->second);
      }
    }
    return *this;
  }
  
  FVector& FVector::operator-= (const FVector& rhs) {
    for (iterator i = begin(); i != end(); ++i) {
      set(i->first,i->second - rhs[i->first]);
    }
    for (const_iterator i = rhs.begin(); i != rhs.end(); ++i) {
      if (!hasValue(i->first)) {
        set(i->first,-(i->second));
      }
    }
    return *this;
  }
  
  FVector& FVector::operator*= (const FValue& rhs) {
    //NB Could do this with boost::bind ?
    for (iterator i = begin(); i != end(); ++i) {
      i->second *= rhs;
    }
    return *this;
  }
  
  
  FVector& FVector::operator/= (const FValue& rhs) {
    for (iterator i = begin(); i != end(); ++i) {
      i->second /= rhs;
    }
    return *this;
  }
  
  const FVector operator+(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) += rhs;
  }
  
  const FVector operator-(const FVector& lhs, const FVector& rhs) {
    return FVector(lhs) -= rhs;
  }
  
  const FVector operator*(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) *= rhs;
  }
  
  const FVector operator/(const FVector& lhs, const FValue& rhs) {
    return FVector(lhs) /= rhs;
  }
  
  FValue operator*(const FVector& lhs, const FVector& rhs) {
    if (lhs.size() >= rhs.size()) {
      FValue prod = 0;
      for (FVector::const_iterator i = lhs.begin(); i != lhs.end(); ++i) {
        prod += i->second * rhs[i->first];
      }
      return prod;
    } else {
      return rhs*lhs;
    }
  }

  
  
}

