#pragma once
#ifndef RULETABLEPARSER_H_INCLUDED_
#define RULETABLEPARSER_H_INCLUDED_

#include <istream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace moses {

class RuleTableParser {
 public:
  struct Entry {
    std::string sourceLhs;
    std::vector<std::string> sourceRhs;
    std::string targetLhs;
    std::vector<std::string> targetRhs;
    std::vector<std::string> scores;
    std::set<std::pair<int, int> > alignments;
    std::vector<std::string> counts;
    std::string tail;
  };

  RuleTableParser();
  RuleTableParser(std::istream &);

  const Entry &operator*() const { return m_value; }
  const Entry *operator->() const { return &m_value; }

  RuleTableParser &operator++();
  RuleTableParser operator++(int);

  friend bool operator==(const RuleTableParser &, const RuleTableParser &);
  friend bool operator!=(const RuleTableParser &, const RuleTableParser &);

 private:
  Entry m_value;
  std::istream *m_input;
  std::string m_line;
  std::vector<std::string> tmpStringVec;

  void parseLine(const std::string &);
  static void trimPairedSymbolFromLeft(std::string &);
  static void trimPairedSymbolFromRight(std::string &);
};

}  // namespace moses

#endif
