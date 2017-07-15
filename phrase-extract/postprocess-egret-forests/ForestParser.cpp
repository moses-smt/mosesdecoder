#include "ForestParser.h"

#include <istream>
#include <string>

#include <boost/make_shared.hpp>

#include "util/tokenize_piece.hh"

#include "syntax-common/exception.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

ForestParser::ForestParser()
  : m_input(0)
{
}

ForestParser::ForestParser(std::istream &input)
  : m_input(&input)
{
  ++(*this);
}

ForestParser &ForestParser::operator++()
{
  if (!m_input) {
    return *this;
  }
  m_vertexSet.clear();
  m_entry.forest.vertices.clear();
  if (!std::getline(*m_input, m_tmpLine)) {
    m_input = 0;
    return *this;
  }
  // The first line contains the sentence number.
  ParseSentenceNumLine(m_tmpLine, m_entry.sentNum);
  // The second line contains the sentence string.
  std::getline(*m_input, m_entry.sentence);
  // Subsequent lines contain hyperedges -- or a blank line if there was a
  // parse failure -- terminated by a blank line.
  std::getline(*m_input, m_tmpLine);
  if (m_tmpLine == "") {  // Parse failure
    std::getline(*m_input, m_tmpLine);
    assert(m_tmpLine == "");
    return *this;
  }
  while (m_tmpLine != "") {
    ParseHyperedgeLine(m_tmpLine, m_entry.forest);
    std::getline(*m_input, m_tmpLine);
  }
  return *this;
}

boost::shared_ptr<Forest::Vertex> ForestParser::AddVertex(const VertexSP &v)
{
  std::pair<VertexSet::iterator, bool> ret = m_vertexSet.insert(v);
  if (ret.second) {
    m_entry.forest.vertices.push_back(*ret.first);
  }
  return *ret.first;
}

void ForestParser::ParseSentenceNumLine(const std::string &line,
                                        std::size_t &sentNum)
{
  const util::AnyCharacter delimiter(" \t");
  util::TokenIter<util::AnyCharacter, true> p(line, delimiter);
  if (*p != "sentence") {
    // FIXME
    throw Exception("");
  }
  ++p;
  std::string tmp;
  p->CopyToString(&tmp);
  sentNum = std::atoi(tmp.c_str());
}

void ForestParser::ParseHyperedgeLine(const std::string &line, Forest &forest)
{
  const util::AnyCharacter delimiter(" \t");
  util::TokenIter<util::AnyCharacter, true> p(line, delimiter);
  VertexSP v = AddVertex(ParseVertex(*p));
  HyperedgeSP e = boost::make_shared<Forest::Hyperedge>();
  e->head = v.get();
  ++p;
  if (*p != "=>") {
    // FIXME
    throw Exception("");
  }
  for (++p; *p != "|||"; ++p) {
    v = ParseVertex(*p);
    if (v->start == -1) {
      // Egret does not give start/end for terminals.
      v->start = v->end = e->head->start;
    }
    e->tail.push_back(AddVertex(v).get());
  }
  ++p;
  std::string tmp;
  p->CopyToString(&tmp);
  e->weight = std::atof(tmp.c_str());
  e->head->incoming.push_back(e);
}

boost::shared_ptr<Forest::Vertex> ForestParser::ParseVertex(
  const StringPiece &s)
{
  VertexSP v = boost::make_shared<Forest::Vertex>();
  std::size_t pos = s.rfind('[');
  if (pos == std::string::npos) {
    s.CopyToString(&v->symbol.value);
    v->symbol.isNonTerminal = false;
    v->start = v->end = -1;
    return v;
  }
  if (pos > 2 && s[pos-2] == '^' && s[pos-1] == 'g') {
    s.substr(0, pos-2).CopyToString(&v->symbol.value);
  } else {
    s.substr(0, pos).CopyToString(&v->symbol.value);
  }
  v->symbol.isNonTerminal = true;
  std::size_t begin = pos + 1;
  pos = s.find(',', begin+1);
  std::string tmp;
  s.substr(begin, pos-begin).CopyToString(&tmp);
  v->start = std::atoi(tmp.c_str());
  s.substr(pos+1, s.size()-pos-2).CopyToString(&tmp);
  v->end = std::atoi(tmp.c_str());
  return v;
}

bool operator==(const ForestParser &lhs, const ForestParser &rhs)
{
  // TODO Is this right?  Compare values of istreams if non-zero?
  return lhs.m_input == rhs.m_input;
}

bool operator!=(const ForestParser &lhs, const ForestParser &rhs)
{
  return !(lhs == rhs);
}

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
