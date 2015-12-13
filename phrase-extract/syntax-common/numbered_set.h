#pragma once

#include <limits>
#include <sstream>
#include <vector>

#include <boost/unordered_map.hpp>

#include "exception.h"

namespace MosesTraining {
namespace Syntax {

// Stores a set of elements of type T, each of which is allocated an integral
// ID of type I.  IDs are contiguous starting at 0.  Individual elements cannot
// be removed once inserted (but the whole set can be cleared).
template<typename T, typename I=size_t>
class NumberedSet {
 private:
  typedef boost::unordered_map<T, I> ElementToIdMap;
  typedef std::vector<const T *> IdToElementMap;

 public:
  typedef I IdType;
  typedef typename IdToElementMap::const_iterator const_iterator;

  NumberedSet() {}

  const_iterator begin() const { return id_to_element_.begin(); }
  const_iterator end() const { return id_to_element_.end(); }

  // Static value
  static I NullId() { return std::numeric_limits<I>::max(); }

  bool IsEmpty() const { return id_to_element_.empty(); }
  size_t Size() const { return id_to_element_.size(); }

  // Insert the given object and return its ID.
  I Insert(const T &);

  // Look up the given object and return its ID.
  I Lookup(const T &) const;

  // Look up the given object using an alternative key type (this is useful if,
  // for example, T is std::string and you want to look up the object using a
  // StringPiece).
  template<typename CompatibleKey, typename CompatibleHash,
           typename CompatiblePredicate>
  I Lookup(const CompatibleKey &, const CompatibleHash &,
           const CompatiblePredicate &) const;

  // Lookup the object with the given ID.
  const T &Lookup(I) const;

  void Clear();

 private:
  ElementToIdMap element_to_id_;
  IdToElementMap id_to_element_;
};

template<typename T, typename I>
I NumberedSet<T, I>::Lookup(const T &s) const {
  typename ElementToIdMap::const_iterator p = element_to_id_.find(s);
  return (p == element_to_id_.end()) ? NullId() : p->second;
}

template<typename T, typename I>
template<typename CompatibleKey, typename CompatibleHash,
         typename CompatiblePredicate>
I NumberedSet<T, I>::Lookup(const CompatibleKey &key,
                            const CompatibleHash &hash,
                            const CompatiblePredicate &pred) const {
  typename ElementToIdMap::const_iterator p =
      element_to_id_.find(key, hash, pred);
  return (p == element_to_id_.end()) ? NullId() : p->second;
}

template<typename T, typename I>
const T &NumberedSet<T, I>::Lookup(I id) const {
  // FIXME Need to check id is > 0 iff I is a signed type.
  //if (id < 0 || id >= id_to_element_.size()) {
  if (id >= id_to_element_.size()) {
    std::ostringstream msg;
    msg << "Value not found: " << id;
    throw Exception(msg.str());
  }
  return *(id_to_element_[id]);
}

template<typename T, typename I>
I NumberedSet<T, I>::Insert(const T &x) {
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
void NumberedSet<T, I>::Clear() {
  element_to_id_.clear();
  id_to_element_.clear();
}

}  // namespace Syntax
}  // namespace MosesTraining
