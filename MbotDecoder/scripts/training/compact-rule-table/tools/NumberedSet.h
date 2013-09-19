#pragma once
#ifndef NUMBEREDSET_H_
#define NUMBEREDSET_H_

#include "Exception.h"

#include <boost/unordered_map.hpp>

#include <limits>
#include <sstream>
#include <vector>

namespace moses {

// Stores a set of elements of type T, each of which is allocated an integral
// ID of type IdType.  IDs are contiguous starting at 0.  Elements cannot be
// removed.
template<typename T, typename IdType=size_t>
class NumberedSet {
 private:
  typedef boost::unordered_map<T, IdType> ElementToIdMap;
  typedef std::vector<const T *> IdToElementMap;

 public:
  typedef typename IdToElementMap::const_iterator const_iterator;

  NumberedSet() {}

  const_iterator begin() const { return m_idToElement.begin(); }
  const_iterator end() const { return m_idToElement.end(); }

  // Static value
  static IdType nullID() { return std::numeric_limits<IdType>::max(); }

  bool empty() const { return m_idToElement.empty(); }
  size_t size() const { return m_idToElement.size(); }

  IdType lookup(const T &) const;
  const T &lookup(IdType) const;

  // Insert the given object and return its ID.
  IdType insert(const T &);

 private:
  ElementToIdMap m_elementToId;
  IdToElementMap m_idToElement;
};

template<typename T, typename IdType>
IdType NumberedSet<T, IdType>::lookup(const T &s) const {
  typename ElementToIdMap::const_iterator p = m_elementToId.find(s);
  return (p == m_elementToId.end()) ? nullID() : p->second;
}

template<typename T, typename IdType>
const T &NumberedSet<T, IdType>::lookup(IdType id) const {
  if (id < 0 || id >= m_idToElement.size()) {
    std::ostringstream msg;
    msg << "Value not found: " << id;
    throw Exception(msg.str());
  }
  return *(m_idToElement[id]);
}

template<typename T, typename IdType>
IdType NumberedSet<T, IdType>::insert(const T &x) {
  std::pair<T, IdType> value(x, m_idToElement.size());
  std::pair<typename ElementToIdMap::iterator, bool> result =
      m_elementToId.insert(value);
  if (result.second) {
    // x is a new element.
    m_idToElement.push_back(&result.first->first);
  }
  return result.first->second;
}

}  // namespace moses

#endif
