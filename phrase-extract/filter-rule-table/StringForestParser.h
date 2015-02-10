#pragma once

#include <istream>
#include <string>
#include <vector>
#include <utility>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

#include "util/string_piece.hh"

#include "StringForest.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

class StringForestParser {
 public:
  struct Entry {
    std::size_t sentNum;
    std::string sentence;
    boost::shared_ptr<StringForest> forest;
  };

  StringForestParser();
  StringForestParser(std::istream &);

  Entry &operator*() { return m_entry; }
  Entry *operator->() { return &m_entry; }

  StringForestParser &operator++();

  friend bool operator==(const StringForestParser &,
                         const StringForestParser &);
  friend bool operator!=(const StringForestParser &,
                         const StringForestParser &);

 private:
  struct VertexSetHash {
    std::size_t operator()(const StringForest::Vertex *v) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, v->value.symbol);
      boost::hash_combine(seed, v->value.start);
      boost::hash_combine(seed, v->value.end);
      return seed;
    }
  };

  struct VertexSetPred {
    bool operator()(const StringForest::Vertex *v,
                    const StringForest::Vertex *w) const {
      return v->value.symbol == w->value.symbol &&
             v->value.start == w->value.start &&
             v->value.end == w->value.end;
    }
  };

  typedef boost::unordered_set<StringForest::Vertex *, VertexSetHash,
                               VertexSetPred> VertexSet;

  // Copying is not allowed
  StringForestParser(const StringForestParser &);
  StringForestParser &operator=(const StringForestParser &);

  StringForest::Vertex *AddOrDeleteVertex(StringForest::Vertex *);
  void ParseHyperedgeLine(const std::string &, StringForest &);
  void ParseSentenceNumLine(const std::string &, std::size_t &);
  StringForest::Vertex *ParseVertex(const StringPiece &);

  Entry m_entry;
  std::istream *m_input;
  std::string m_tmpLine;
  VertexSet m_vertexSet;
};

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
