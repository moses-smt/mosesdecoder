#pragma once

#include <string>

#include <boost/functional/hash.hpp>

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

struct Symbol {
  std::string value;
  bool isNonTerminal;
};

inline std::size_t hash_value(const Symbol &s)
{
  std::size_t seed = 0;
  boost::hash_combine(seed, s.value);
  boost::hash_combine(seed, s.isNonTerminal);
  return seed;
}

inline bool operator==(const Symbol &s, const Symbol &t)
{
  return s.value == t.value && s.isNonTerminal == t.isNonTerminal;
}

struct SymbolHasher {
public:
  std::size_t operator()(const Symbol &s) const {
    return hash_value(s);
  }
};

struct SymbolEqualityPred {
public:
  bool operator()(const Symbol &s, const Symbol &t) const {
    return s.value == t.value && s.isNonTerminal == t.isNonTerminal;
  }
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
