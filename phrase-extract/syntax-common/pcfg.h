#pragma once

#include <istream>
#include <map>
#include <ostream>
#include <vector>

#include "vocabulary.h"

namespace MosesTraining {
namespace Syntax {

class Pcfg {
 public:
  typedef std::vector<std::size_t> Key;
  typedef std::map<Key, double> Map;
  typedef Map::iterator iterator;
  typedef Map::const_iterator const_iterator;

  Pcfg() {}

  iterator begin() { return rules_.begin(); }
  const_iterator begin() const { return rules_.begin(); }

  iterator end() { return rules_.end(); }
  const_iterator end() const { return rules_.end(); }

  void Add(const Key &, double);
  bool Lookup(const Key &, double &) const;
  void Read(std::istream &, Vocabulary &);
  void Write(const Vocabulary &, std::ostream &) const;

 private:
  Map rules_;
};

}  // namespace Syntax
}  // namespace MosesTraining
