#include "StringForestParser.h"

#include <istream>
#include <string>

#include <boost/make_shared.hpp>

#include "util/tokenize_piece.hh"

#include "syntax-common/exception.h"

namespace MosesTraining
{
namespace Syntax
{
namespace FilterRuleTable
{

StringForestParser::StringForestParser()
    : m_input(0) {
}

StringForestParser::StringForestParser(std::istream &input)
    : m_input(&input) {
  ++(*this);
}

StringForestParser &StringForestParser::operator++() {
  if (!m_input) {
    return *this;
  }
  m_vertexSet.clear();
  m_entry.forest.reset(new StringForest());
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
    ParseHyperedgeLine(m_tmpLine, *m_entry.forest);
    std::getline(*m_input, m_tmpLine);
  }
  return *this;
}

StringForest::Vertex *StringForestParser::AddOrDeleteVertex(
    StringForest::Vertex *v)
{
  std::pair<VertexSet::iterator, bool> ret = m_vertexSet.insert(v);
  if (ret.second) {
    m_entry.forest->vertices.push_back(*ret.first);
  } else {
    delete v;
  }
  return *ret.first;
}

void StringForestParser::ParseSentenceNumLine(const std::string &line,
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

void StringForestParser::ParseHyperedgeLine(const std::string &line,
                                            StringForest &forest)
{
  const util::AnyCharacter delimiter(" \t");
  util::TokenIter<util::AnyCharacter, true> p(line, delimiter);
  StringForest::Vertex *v = AddOrDeleteVertex(ParseVertex(*p));
  StringForest::Hyperedge *e = new StringForest::Hyperedge();
  e->head = v;
  ++p;
  if (*p != "=>") {
    // FIXME
    throw Exception("");
  }
  for (++p; *p != "|||"; ++p) {
    v = ParseVertex(*p);
    if (v->value.start == -1) {
      // Egret does not give start/end for terminals.
      v->value.start = v->value.end = e->head->value.start;
    }
    e->tail.push_back(AddOrDeleteVertex(v));
  }
  // Weight is ignored
  e->head->incoming.push_back(e);
}

StringForest::Vertex *StringForestParser::ParseVertex(const StringPiece &s)
{
  StringForest::Vertex *v = new StringForest::Vertex();
  std::size_t pos = s.rfind('[');
  if (pos == std::string::npos) {
    s.CopyToString(&v->value.symbol);
    //v.value.symbol.isNonTerminal = false;
    v->value.start = v->value.end = -1;
    return v;
  }
  if (pos > 2 && s[pos-2] == '^' && s[pos-1] == 'g') {
    s.substr(0, pos-2).CopyToString(&v->value.symbol);
  } else {
    s.substr(0, pos).CopyToString(&v->value.symbol);
  }
  //v.symbol.isNonTerminal = true;
  std::size_t begin = pos + 1;
  pos = s.find(',', begin+1);
  std::string tmp;
  s.substr(begin, pos-begin).CopyToString(&tmp);
  v->value.start = std::atoi(tmp.c_str());
  s.substr(pos+1, s.size()-pos-2).CopyToString(&tmp);
  v->value.end = std::atoi(tmp.c_str());
  return v;
}

bool operator==(const StringForestParser &lhs, const StringForestParser &rhs) {
  // TODO Is this right?  Compare values of istreams if non-zero?
  return lhs.m_input == rhs.m_input;
}

bool operator!=(const StringForestParser &lhs, const StringForestParser &rhs) {
  return !(lhs == rhs);
}

}  // namespace FilterRuleTable
}  // namespace Syntax
}  // namespace MosesTraining
