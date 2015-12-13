#pragma once

#include <istream>
#include <string>
#include <vector>
#include <utility>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>

#include "util/string_piece.hh"

#include "Forest.h"
#include "Symbol.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

class ForestParser
{
public:
  struct Entry {
    std::size_t sentNum;
    std::string sentence;
    Forest forest;
  };

  ForestParser();
  ForestParser(std::istream &);

  Entry &operator*() {
    return m_entry;
  }
  Entry *operator->() {
    return &m_entry;
  }

  ForestParser &operator++();

  friend bool operator==(const ForestParser &, const ForestParser &);
  friend bool operator!=(const ForestParser &, const ForestParser &);

private:
  typedef boost::shared_ptr<Forest::Vertex> VertexSP;
  typedef boost::shared_ptr<Forest::Hyperedge> HyperedgeSP;

  struct VertexSetHash {
    std::size_t operator()(const VertexSP &v) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, v->symbol);
      boost::hash_combine(seed, v->start);
      boost::hash_combine(seed, v->end);
      return seed;
    }
  };

  struct VertexSetPred {
    bool operator()(const VertexSP &v, const VertexSP &w) const {
      return v->symbol == w->symbol && v->start == w->start && v->end == w->end;
    }
  };

  typedef boost::unordered_set<VertexSP, VertexSetHash,
          VertexSetPred> VertexSet;

  // Copying is not allowed
  ForestParser(const ForestParser &);
  ForestParser &operator=(const ForestParser &);

  VertexSP AddVertex(const VertexSP &);
  void ParseHyperedgeLine(const std::string &, Forest &);
  void ParseSentenceNumLine(const std::string &, std::size_t &);
  VertexSP ParseVertex(const StringPiece &);

  Entry m_entry;
  std::istream *m_input;
  std::string m_tmpLine;
  VertexSet m_vertexSet;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
