#pragma once

#include <vector>
#include <map>
#include <string>

class Parameter;

class SyntaxTree
{
public:
  typedef std::pair<int, int> Range;
  typedef std::vector<std::string> Labels;
  typedef std::map<Range, Labels> Coll;

  void Add(int startPos, int endPos, const std::string &label, const Parameter &params);
  void AddToAll(const std::string &label);

  const Labels &Find(int startPos, int endPos) const;

  void SetHieroLabel(const std::string &label) {
    m_defaultLabels.push_back(label);
  }


protected:

  Coll m_coll;
  Labels m_defaultLabels;
};


