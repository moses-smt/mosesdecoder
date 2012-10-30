#pragma once

#include <string>
#include <vector>
#include <map>

class Alignments
{
public:
  std::vector< std::map<int, int> > m_alignS2T, m_alignT2S;

  Alignments(const std::string &align, size_t sourceSize, size_t targetSize);


protected:

};



