// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011- University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef _FEATURE_DATA_ITERATOR_
#define _FEATURE_DATA_ITERATOR_

/**
  * For loading from the feature data file.
**/

#include <fstream>
#include <map>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>


//Minimal sparse vector
class SparseVector {

  public:
    typedef std::map<size_t,float> fvector_t;
    typedef std::map<std::string, size_t> name2id_t;
    typedef std::vector<std::string> id2name_t;

    float get(std::string name) const;
    float get(size_t id) const;
    void set(std::string name, float value);
    void clear();
    size_t size() const;

    void write(std::ostream& out, const std::string& sep = " ") const;

    SparseVector& operator-=(const SparseVector& rhs);

  private:
    static name2id_t name2id_;
    static id2name_t id2name_;
    fvector_t fvector_;
};

SparseVector operator-(const SparseVector& lhs, const SparseVector& rhs);

class FeatureDataItem {
  public:
    std::vector<float> dense;
    SparseVector sparse;
};

class FeatureDataIterator : 
  public boost::iterator_facade<FeatureDataIterator,
                                const std::vector<FeatureDataItem>,
                                boost::forward_traversal_tag> 
{
  public:
    FeatureDataIterator(const std::string filename);

    static FeatureDataIterator end() {
      return FeatureDataIterator("");
    }


  private:
    friend class boost::iterator_core_access;

    void increment();
    bool equal(const FeatureDataIterator& rhs) const;
    const std::vector<FeatureDataItem>& dereference() const;

    std::ifstream* in_;
};

#endif


