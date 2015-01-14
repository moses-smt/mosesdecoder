#pragma once

#include <vector>

#include <boost/unordered_map.hpp>

#include "moses/FactorCollection.h"
#include "moses/Word.h"

#include "SymbolEqualityPred.h"
#include "SymbolHasher.h"

namespace Moses
{
namespace Syntax
{

// Hybrid map/vector-based container for key-value pairs where the key is a
// non-terminal Word.  The interface is like a (stripped-down) map type, with
// the main differences being that:
//   1. Find() is implemented using vector indexing to make it fast.
//   2. Once a value has been inserted it can be modified but can't be removed.
template<typename T>
class NonTerminalMap
{
private:
  typedef boost::unordered_map<Word, T, SymbolHasher, SymbolEqualityPred> Map;
  typedef std::vector<T*> Vec;

public:
  typedef typename Map::iterator Iterator;
  typedef typename Map::const_iterator ConstIterator;

  NonTerminalMap()
    : m_vec(FactorCollection::Instance().GetNumNonTerminals(), NULL) {}

  Iterator Begin() {
    return m_map.begin();
  }
  Iterator End() {
    return m_map.end();
  }

  ConstIterator Begin() const {
    return m_map.begin();
  }
  ConstIterator End() const {
    return m_map.end();
  }

  std::size_t Size() const {
    return m_map.size();
  }

  bool IsEmpty() const {
    return m_map.empty();
  }

  std::pair<Iterator, bool> Insert(const Word &, const T &);

  T *Find(const Word &w) const {
    return m_vec[w[0]->GetId()];
  }

private:
  Map m_map;
  Vec m_vec;
};

template<typename T>
std::pair<typename NonTerminalMap<T>::Iterator, bool> NonTerminalMap<T>::Insert(
  const Word &key, const T &value)
{
  std::pair<typename Map::iterator, bool> result =
    m_map.insert(typename Map::value_type(key, value));
  if (result.second) {
    T *p = &(result.first->second);
    std::size_t i = key[0]->GetId();
    m_vec[i] = p;
  }
  return result;
}

}  // namespace Syntax
}  // namespace Moses
