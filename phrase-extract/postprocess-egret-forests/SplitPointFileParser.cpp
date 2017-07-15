#include "SplitPointFileParser.h"

#include <istream>
#include <string>

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "syntax-common/exception.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

SplitPointFileParser::SplitPointFileParser()
  : m_input(0)
{
}

SplitPointFileParser::SplitPointFileParser(std::istream &input)
  : m_input(&input)
{
  ++(*this);
}

SplitPointFileParser &SplitPointFileParser::operator++()
{
  if (!m_input) {
    return *this;
  }
  m_entry.splitPoints.clear();
  if (!std::getline(*m_input, m_tmpLine)) {
    m_input = 0;
    return *this;
  }
  ParseLine(m_tmpLine, m_entry.splitPoints);
  return *this;
}

void SplitPointFileParser::ParseLine(const std::string &line,
                                     std::vector<SplitPoint> &splitPoints)
{
  std::string tmp;
  const util::AnyCharacter delimiter(" \t");
  for (util::TokenIter<util::AnyCharacter, true> p(line, delimiter); p; ++p) {
    splitPoints.resize(splitPoints.size()+1);
    SplitPoint &splitPoint = splitPoints.back();
    std::size_t pos = p->find(',');

    StringPiece sp = p->substr(0, pos);
    sp.CopyToString(&tmp);
    splitPoint.tokenPos = std::atoi(tmp.c_str());
    std::size_t begin = pos+1;
    pos = p->find(',', begin);

    sp = p->substr(begin, pos-begin);
    sp.CopyToString(&tmp);
    splitPoint.charPos = std::atoi(tmp.c_str());

    sp = p->substr(pos+1);
    sp.CopyToString(&splitPoint.connector);
    if (splitPoint.connector.size() > 1) {
      throw Exception("multi-character connectors not currently supported");
    }
  }
}

bool operator==(const SplitPointFileParser &lhs,
                const SplitPointFileParser &rhs)
{
  // TODO Is this right?  Compare values of istreams if non-zero?
  return lhs.m_input == rhs.m_input;
}

bool operator!=(const SplitPointFileParser &lhs,
                const SplitPointFileParser &rhs)
{
  return !(lhs == rhs);
}

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
