#include "RuleTableParser.h"

#include "Exception.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

#include <istream>
#include <string>

namespace moses {

RuleTableParser::RuleTableParser()
  : m_input(0) {
}

RuleTableParser::RuleTableParser(std::istream &input)
  : m_input(&input) {
  ++(*this);
}

RuleTableParser & RuleTableParser::operator++() {
  if (!m_input) {
    return *this;
  }
  if (!std::getline(*m_input, m_line)) {
    m_input = 0;
    return *this;
  }
  parseLine(m_line);
  return *this;
}

RuleTableParser RuleTableParser::operator++(int) {
  RuleTableParser tmp(*this);
  ++(*this);
  return tmp;
}

void RuleTableParser::parseLine(const std::string &line) {
  // Source symbols
  size_t pos = line.find("|||");
  if (pos == std::string::npos) {
    throw Exception("missing first delimiter");
  }
  std::string text = line.substr(0, pos);
  boost::trim(text);
  m_value.sourceRhs.clear();
  boost::split(m_value.sourceRhs, text, boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
  m_value.sourceLhs = m_value.sourceRhs.back();
  m_value.sourceRhs.pop_back();
  std::for_each(m_value.sourceRhs.begin(), m_value.sourceRhs.end(),
                trimPairedSymbolFromRight);

  // Target symbols
  size_t begin = pos+3;
  pos = line.find("|||", begin);
  if (pos == std::string::npos) {
    throw Exception("missing second delimiter");
  }
  text = line.substr(begin, pos-begin);
  boost::trim(text);
  m_value.targetRhs.clear();
  boost::split(m_value.targetRhs, text, boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
  m_value.targetLhs = m_value.targetRhs.back();
  m_value.targetRhs.pop_back();
  std::for_each(m_value.targetRhs.begin(), m_value.targetRhs.end(),
                trimPairedSymbolFromLeft);

  // Scores
  begin = pos+3;
  pos = line.find("|||", begin);
  if (pos == std::string::npos) {
    throw Exception("missing third delimiter");
  }
  text = line.substr(begin, pos-begin);
  boost::trim(text);
  m_value.scores.clear();
  boost::split(m_value.scores, text, boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);

  // Alignments
  begin = pos+3;
  pos = line.find("|||", begin);
  if (pos == std::string::npos) {
    throw Exception("missing fourth delimiter");
  }
  text = line.substr(begin, pos-begin);
  m_value.alignments.clear();
  boost::trim(text);
  // boost::split behaves differently between versions on empry strings
  if (!text.empty()) {
    tmpStringVec.clear();
    boost::split(tmpStringVec, text, boost::algorithm::is_space(),
                 boost::algorithm::token_compress_on);
    for (std::vector<std::string>::const_iterator p = tmpStringVec.begin();
         p != tmpStringVec.end(); ++p) {
      assert(!p->empty());
      std::vector<std::string> tmpVec;
      tmpVec.reserve(2);
      boost::split(tmpVec, *p, boost::algorithm::is_any_of("-"));
      if (tmpVec.size() != 2) {
        throw Exception("bad alignment pair");
      }
      std::pair<int, int> alignmentPair;
      alignmentPair.first = boost::lexical_cast<int>(tmpVec[0]);
      alignmentPair.second = boost::lexical_cast<int>(tmpVec[1]);
      m_value.alignments.insert(alignmentPair);
    }
  }

  // Counts + everything else (the 'tail')
  begin = pos+3;
  pos = line.find("|||", begin);
  if (pos == std::string::npos) {
    text = line.substr(begin);
    m_value.tail.clear();
  } else {
    text = line.substr(begin, pos-begin);
    m_value.tail = line.substr(pos+3);
  }
  boost::trim(text);
  m_value.counts.clear();
  boost::split(m_value.counts, text, boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
}

void RuleTableParser::trimPairedSymbolFromLeft(std::string &s) {
  size_t len = s.size();
  if (len < 2 || s[0] != '[' || s[len-1] != ']') {
    return;
  }
  size_t pos = s.find('[', 1);
  if (pos == std::string::npos) {
    std::ostringstream msg;
    msg << "malformed non-terminal pair: " << s;
    throw Exception(msg.str());
  }
  s.erase(0, pos);
}

void RuleTableParser::trimPairedSymbolFromRight(std::string &s) {
  size_t len = s.size();
  if (len < 2 || s[0] != '[' || s[len-1] != ']') {
    return;
  }
  size_t pos = s.find('[', 1);
  if (pos == std::string::npos) {
    std::ostringstream msg;
    msg << "malformed non-terminal pair: " << s;
    throw Exception(msg.str());
  }
  s.resize(pos);
}

bool operator==(const RuleTableParser &lhs, const RuleTableParser &rhs) {
  return lhs.m_input == rhs.m_input;
}

bool operator!=(const RuleTableParser &lhs, const RuleTableParser &rhs) {
  return !(lhs == rhs);
}

}  // namespace moses
