// -*- c++ -*-
#ifndef moses_ForestInput_h
#define moses_ForestInput_h

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <util/string_piece.hh>

#include "moses/Syntax/F2S/Forest.h"

#include "Sentence.h"

namespace Moses
{
class TranslationTask;
class ForestInput : public Sentence
{
public:
  friend std::ostream &operator<<(std::ostream&, const ForestInput &);

  ForestInput(AllOptions::ptr const& opts) : Sentence(opts), m_rootVertex(NULL) {}

  InputTypeEnum GetType() const {
    return ForestInputType;
  }

  //! populate this InputType with data from in stream
  virtual int
  Read(std::istream& in);

  //! Output debugging info to stream out
  virtual void Print(std::ostream&) const;

  //! create trans options specific to this InputType
  virtual TranslationOptionCollection*
  CreateTranslationOptionCollection() const;

  boost::shared_ptr<const Syntax::F2S::Forest> GetForest() const {
    return m_forest;
  }

  const Syntax::F2S::Forest::Vertex *GetRootVertex() const {
    return m_rootVertex;
  }

private:
  typedef Syntax::F2S::Forest Forest;

  struct VertexSetHash {
    std::size_t operator()(const Forest::Vertex *v) const {
      std::size_t seed = 0;
      boost::hash_combine(seed, v->pvertex.symbol);
      boost::hash_combine(seed, v->pvertex.span.GetStartPos());
      boost::hash_combine(seed, v->pvertex.span.GetEndPos());
      return seed;
    }
  };

  struct VertexSetPred {
    bool operator()(const Forest::Vertex *v, const Forest::Vertex *w) const {
      return v->pvertex == w->pvertex;
    }
  };

  typedef boost::unordered_set<Forest::Vertex *, VertexSetHash,
          VertexSetPred> VertexSet;

  Forest::Vertex *AddOrDeleteVertex(Forest::Vertex *);

  std::size_t FindMaxEnd(const Forest &);

  void FindTopVertices(Forest &, std::vector<Forest::Vertex *> &);

  void ParseHyperedgeLine(const std::string &);

  Forest::Vertex *ParseVertex(const StringPiece &);

  boost::shared_ptr<Forest> m_forest;
  Forest::Vertex *m_rootVertex;
  VertexSet m_vertexSet;
};

}  // namespace Moses

#endif
