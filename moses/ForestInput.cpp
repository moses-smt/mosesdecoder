#include "ForestInput.h"

#include <algorithm>

#include <boost/make_shared.hpp>

#include "util/tokenize_piece.hh"

#include "moses/Syntax/F2S/Forest.h"
#include "moses/TranslationModel/PhraseDictionary.h"

#include "FactorCollection.h"
#include "StaticData.h"
#include "Util.h"

namespace Moses
{

//! populate this InputType with data from in stream
int ForestInput::
Read(std::istream &in)
{
  using Syntax::F2S::Forest;

  m_forest = boost::make_shared<Forest>();
  m_rootVertex = NULL;
  m_vertexSet.clear();

  std::string line;
  if (std::getline(in, line, '\n').eof()) {
    return 0;
  }

  // The first line contains the sentence number.  We ignore this and skip
  // straight to the second line, which contains the sentence string.
  std::string sentence;
  std::getline(in, sentence);

  // If the next line is blank then there was a parse failure.  Otherwise,
  // the next line and any subsequent non-blank lines contain hyperedges.
  std::getline(in, line);
  if (line == "") {
    // Parse failure.  We treat this as an empty sentence.
    sentence = "";
    // The next line will be blank too.
    std::getline(in, line);
  } else {
    do {
      ParseHyperedgeLine(line);
      std::getline(in, line);
    } while (line != "");
  }

  // Do base class Read().
  // TODO Check if this is actually necessary.  TreeInput does it, but I'm
  // not sure ForestInput needs to.
  std::stringstream strme;
  strme << "<s> " << sentence << " </s>" << std::endl;
  Sentence::Read(strme);

  // Find the maximum end position of any vertex (0 if forest is empty).
  std::size_t maxEnd = FindMaxEnd(*m_forest);

  // Determine which vertices are the top vertices.
  std::vector<Forest::Vertex *> topVertices;
  if (!m_forest->vertices.empty()) {
    FindTopVertices(*m_forest, topVertices);
    assert(topVertices.size() >= 1);
  }


  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;

  // Add <s> vertex.
  Forest::Vertex *startSymbol = NULL;
  {
    Word symbol;
    symbol.CreateFromString(Input, factorOrder, "<s>", false);
    Syntax::PVertex pvertex(Range(0, 0), symbol);
    startSymbol = new Forest::Vertex(pvertex);
    m_forest->vertices.push_back(startSymbol);
  }

  // Add </s> vertex.
  Forest::Vertex *endSymbol = NULL;
  {
    Word symbol;
    symbol.CreateFromString(Input, factorOrder, "</s>", false);
    Syntax::PVertex pvertex(Range(maxEnd+1, maxEnd+1), symbol);
    endSymbol = new Forest::Vertex(pvertex);
    m_forest->vertices.push_back(endSymbol);
  }

  // Add root vertex.
  {
    Word symbol;
    symbol.CreateFromString(Input, factorOrder, "Q", true);
    Syntax::PVertex pvertex(Range(0, maxEnd+1), symbol);
    m_rootVertex = new Forest::Vertex(pvertex);
    m_forest->vertices.push_back(m_rootVertex);
  }

  // Add root's incoming hyperedges.
  if (topVertices.empty()) {
    Forest::Hyperedge *e = new Forest::Hyperedge();
    e->head = m_rootVertex;
    e->tail.push_back(startSymbol);
    e->tail.push_back(endSymbol);
    m_rootVertex->incoming.push_back(e);
  } else {
    // Add a hyperedge between [Q] and each top vertex.
    for (std::vector<Forest::Vertex *>::const_iterator
         p = topVertices.begin(); p != topVertices.end(); ++p) {
      Forest::Hyperedge *e = new Forest::Hyperedge();
      e->head = m_rootVertex;
      e->tail.push_back(startSymbol);
      e->tail.push_back(*p);
      e->tail.push_back(endSymbol);
      m_rootVertex->incoming.push_back(e);
    }
  }

  return 1;
}

Syntax::F2S::Forest::Vertex*
ForestInput::
AddOrDeleteVertex(Forest::Vertex *v)
{
  std::pair<VertexSet::iterator, bool> ret = m_vertexSet.insert(v);
  if (ret.second) {
    m_forest->vertices.push_back(*ret.first);
  } else {
    delete v;
  }
  return *ret.first;
}

std::size_t ForestInput::FindMaxEnd(const Forest &forest)
{
  std::size_t maxEnd = 0;
  for (std::vector<Forest::Vertex *>::const_iterator
       p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
    maxEnd = std::max(maxEnd, (*p)->pvertex.span.GetEndPos());
  }
  return maxEnd;
}

void ForestInput::FindTopVertices(Forest &forest,
                                  std::vector<Forest::Vertex *> &topVertices)
{
  topVertices.clear();

  // The set of all vertices.
  std::set<Forest::Vertex *> all;

  // The set of all vertices that are the predecessor of another vertex.
  std::set<Forest::Vertex *> preds;

  // Populate the all and preds sets.
  for (std::vector<Forest::Vertex *>::const_iterator
       p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
    all.insert(*p);
    for (std::vector<Forest::Hyperedge *>::const_iterator
         q = (*p)->incoming.begin(); q != (*p)->incoming.end(); ++q) {
      for (std::vector<Forest::Vertex*>::const_iterator
           r = (*q)->tail.begin(); r != (*q)->tail.end(); ++r) {
        preds.insert(*r);
      }
    }
  }

  // The top vertices are the vertices that are in all but not in preds.
  std::set_difference(all.begin(), all.end(), preds.begin(), preds.end(),
                      std::back_inserter(topVertices));
}

void
ForestInput::
ParseHyperedgeLine(const std::string &line)
{
  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;
  using Syntax::F2S::Forest;

  const util::AnyCharacter delimiter(" \t");
  util::TokenIter<util::AnyCharacter, true> p(line, delimiter);
  Forest::Vertex *v = AddOrDeleteVertex(ParseVertex(*p));
  Forest::Hyperedge *e = new Forest::Hyperedge();
  e->head = v;
  ++p;
  if (*p != "=>") {
    // FIXME
    //throw Exception("");
  }
  for (++p; *p != "|||"; ++p) {
    v = ParseVertex(*p);
    if (!v->pvertex.symbol.IsNonTerminal()) {
      // Egret does not give start/end for terminals.
      v->pvertex.span = Range(e->head->pvertex.span.GetStartPos(),
                              e->head->pvertex.span.GetStartPos());
    }
    e->tail.push_back(AddOrDeleteVertex(v));
  }
  ++p;
  std::string tmp;
  p->CopyToString(&tmp);
  e->weight = std::atof(tmp.c_str());
  e->head->incoming.push_back(e);
}

Syntax::F2S::Forest::Vertex*
ForestInput::ParseVertex(const StringPiece &s)
{
  using Syntax::F2S::Forest;
  const std::vector<FactorType>& factorOrder = m_options->input.factor_order;
  Word symbol;
  std::size_t pos = s.rfind('[');
  if (pos == std::string::npos) {
    symbol.CreateFromString(Input, factorOrder, s, false);
    // Create vertex: caller will fill in span.
    Range span(0, 0);
    return new Forest::Vertex(Syntax::PVertex(span, symbol));
  }
  symbol.CreateFromString(Input, factorOrder, s.substr(0, pos), true);
  std::size_t begin = pos + 1;
  pos = s.find(',', begin+1);
  std::string tmp;
  s.substr(begin, pos-begin).CopyToString(&tmp);
  std::size_t start = std::atoi(tmp.c_str());
  s.substr(pos+1, s.size()-pos-2).CopyToString(&tmp);
  std::size_t end = std::atoi(tmp.c_str());
  // Create vertex: offset span by 1 to allow for <s> in first position.
  Range span(start+1, end+1);
  return new Forest::Vertex(Syntax::PVertex(span, symbol));
}

//! Output debugging info to stream out
void ForestInput::Print(std::ostream &out) const
{
  out << *this << "\n";
}

//! create trans options specific to this InputType
TranslationOptionCollection* ForestInput::
CreateTranslationOptionCollection() const
{

  return NULL;
}

// FIXME
std::ostream& operator<<(std::ostream &out, const ForestInput &)
{
  return out;
}

} // namespace Moses
