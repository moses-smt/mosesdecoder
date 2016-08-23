#pragma once
#include <vector>
#include <string>
#include <iostream>

class Rec
{
public:
  float prob;
  std::string line;

  Rec(float aprob, const std::string &aline)
    :prob(aprob)
    ,line(aline)
  {}

  inline bool operator< (const Rec &compare) const {
    return prob < compare.prob;
  }
};

////////////////////////////////////////////////////////////

void Process(int limit, std::istream &inStrme, std::ostream &outStrme);
void Output(std::ostream &outStrme, std::vector<Rec> &records, int limit);

////////////////////////////////////////////////////////////
inline void Tokenize(std::vector<std::string> &output
                     , const std::string& str
                     , const std::string& delimiters = " \t")
{
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    // Found a token, add it to the vector.
    output.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}

