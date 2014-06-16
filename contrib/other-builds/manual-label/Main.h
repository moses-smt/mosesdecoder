#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <list>

typedef std::vector<std::string> Word;
typedef std::vector<Word> Phrase;

struct Range
{
  Range(int start,int end, const std::string &l)
  :range(start, end)
  ,label(l)
  {}

  std::pair<int,int> range;
  std::string label;
};

typedef std::list<Range> Ranges;

bool IsA(const Phrase &source, int pos, int offset, int factor, const std::string &str);
void OutputWithLabels(const Phrase &source, const Ranges ranges, std::ostream &out);


