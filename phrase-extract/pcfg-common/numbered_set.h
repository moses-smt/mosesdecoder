/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh

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

#pragma once
#ifndef PCFG_NUMBERED_SET_H_
#define PCFG_NUMBERED_SET_H_

#include "exception.h"

#include <boost/unordered_map.hpp>

#include <limits>
#include <sstream>
#include <vector>

namespace Moses
{
namespace PCFG
{

// Stores a set of elements of type T, each of which is allocated an integral
// ID of type I.  IDs are contiguous starting at 0.  Individual elements cannot
// be removed once inserted (but the whole set can be cleared).
template<typename T, typename I=std::size_t>
class NumberedSet
{
private:
  typedef boost::unordered_map<T, I> ElementToIdMap;
  typedef std::vector<const T *> IdToElementMap;

public:
  typedef I IdType;
  typedef typename IdToElementMap::const_iterator const_iterator;

  NumberedSet() {}

  const_iterator begin() const {
    return id_to_element_.begin();
  }
  const_iterator end() const {
    return id_to_element_.end();
  }

  // Static value
  static I NullId() {
    return std::numeric_limits<I>::max();
  }

  bool Empty() const {
    return id_to_element_.empty();
  }
  std::size_t Size() const {
    return id_to_element_.size();
  }

  // Insert the given object and return its ID.
  I Insert(const T &);

  I Lookup(const T &) const;
  const T &Lookup(I) const;

  void Clear();

private:
  ElementToIdMap element_to_id_;
  IdToElementMap id_to_element_;
};

template<typename T, typename I>
I NumberedSet<T, I>::Lookup(const T &s) const
{
  typename ElementToIdMap::const_iterator p = element_to_id_.find(s);
  return (p == element_to_id_.end()) ? NullId() : p->second;
}

template<typename T, typename I>
const T &NumberedSet<T, I>::Lookup(I id) const
{
  if (id < 0 || id >= id_to_element_.size()) {
    std::ostringstream msg;
    msg << "Value not found: " << id;
    throw Exception(msg.str());
  }
  return *(id_to_element_[id]);
}

template<typename T, typename I>
I NumberedSet<T, I>::Insert(const T &x)
{
  std::pair<T, I> value(x, id_to_element_.size());
  std::pair<typename ElementToIdMap::iterator, bool> result =
    element_to_id_.insert(value);
  if (result.second) {
    // x is a new element.
    id_to_element_.push_back(&result.first->first);
  }
  return result.first->second;
}

template<typename T, typename I>
void NumberedSet<T, I>::Clear()
{
  element_to_id_.clear();
  id_to_element_.clear();
}

}  // namespace PCFG
}  // namespace Moses

#endif
