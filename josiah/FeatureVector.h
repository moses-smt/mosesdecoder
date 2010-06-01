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

#ifndef FEATUREVECTOR_H
#define FEATUREVECTOR_H

#include <iostream>
#include <map>
#include <string>

namespace Josiah {

typedef float FValue;

/**
  * Feature name
**/
struct FName {
  FName(const std::string theRoot, const std::string theName) 
      : root(theRoot), name(theName) {}
  const std::string root;
  const std::string name;
};

std::ostream& operator<<(std::ostream& out,const FName& name);

struct FNameComparator {
  inline bool operator() (const FName& lhs, const FName& rhs) const {
    if (lhs.root < rhs.root) return true;
    else if (rhs.root < lhs.root) return false;
    else return  (lhs.name < rhs.name);
  }
};

class ProxyFVector;




/**
  * A sparse feature (or weight) vector.
  **/
class FVector
{
  public:
    /** Empty feature vector */
    FVector();
    
    /** Load from file - each line should be 'root[_name] value' */
    void load(const std::string& filename);
    
    /** Element access */
    ProxyFVector operator[](const FName& name);
    const FValue& operator[](const FName& name) const;
    bool hasValue(const FName& name) const;
    
    /** Size */
    size_t size() const {return m_features.size();}
    
    /** Iterators */
    typedef std::map<FName,FValue>::iterator iterator;
    typedef std::map<FName,FValue>::const_iterator const_iterator;
    iterator begin() {return m_features.begin();}
    iterator end() {return m_features.end();}
    const_iterator begin() const {return m_features.begin();}
    const_iterator end() const {return m_features.end();}
    
    friend class ProxyFVector;
    
    /**arithmetic */
    FVector& operator+= (const FVector& rhs);
    FVector& operator-= (const FVector& rhs);
    FVector& operator*= (const FValue& rhs);
    FVector& operator/= (const FValue& rhs);
    
    /** printing */
    std::ostream& print(std::ostream& out) const;
    
    
    
  private:
    const FValue& get(const FName& name) const;
    void set(const FName& name, const FValue& value);
    
    static FValue DEFAULT;

    
    std::map<FName,FValue,FNameComparator> m_features;
    
};

std::ostream& operator<<( std::ostream& out, const FVector& fv);
const FVector operator+(const FVector& lhs, const FVector& rhs);
const FVector operator-(const FVector& lhs, const FVector& rhs);
const FVector operator*(const FVector& lhs, const FValue& rhs);
const FVector operator/(const FVector& lhs, const FValue& rhs);
//inner product.
FValue operator*(const FVector& lhs, const FVector& rhs);

/**
  * Used to help with subscript operator overloading.
  * See http://stackoverflow.com/questions/1386075/overloading-operator-for-a-sparse-vector
  **/
class ProxyFVector {
  public:
    ProxyFVector(FVector *fv, const FName& name ) : m_fv(fv), m_name(name) {}
    ProxyFVector &operator=(const FValue& value) {
      // If we get here, we know that operator[] was called to perform a write access,
      // so we can insert an item in the vector if needed
      //std::cerr << "Inserting " << value << " into " << m_name << std::endl;
      m_fv->set(m_name,value);
      return *this;
      
    }

    operator FValue() {
      // If we get here, we know that operator[] was called to perform a read access,
      // so we can simply return the value from the vector
      return m_fv->get(m_name);
    }
  private:
    FVector* m_fv;
    const FName& m_name;
    
};



}

#endif // FEATUREVECTOR_H
