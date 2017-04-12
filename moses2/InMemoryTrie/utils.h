#pragma once

#include "InMemoryTrie.h"
#include <fstream>
#include <ostream>
#include <string>
#include <vector>
#include "legacy/Util2.h"
#include "../legacy/Factor.h"
#include "../legacy/InputFileStream.h"

using namespace std;

namespace Moses2
{

inline void ParseLineByChar(string& line, char c, vector<string>& substrings)
{
  size_t i = 0;
  size_t j = line.find(c);

  while (j != string::npos) {
    substrings.push_back(line.substr(i, j - i));
    i = ++j;
    j = line.find(c, j);

    if (j == string::npos) substrings.push_back(line.substr(i, line.length()));
  }
}

}

