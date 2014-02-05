#pragma once

#include <iostream>
#include <vector>
#include <string>

typedef std::vector<std::string> Word;
typedef std::vector<Word> Phrase;

typedef std::pair<int,int> Range;
typedef std::list<Range> Ranges;

bool IsA(const Phrase &source, int pos, int offset, int factor, const std::string &str);
void OutputWithLabels(const Phrase &source, const Ranges ranges, std::ostream &out);


