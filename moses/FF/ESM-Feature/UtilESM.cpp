#include "UtilESM.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>

namespace Moses
{

bool overlap(const MinPhrase& p1, const MinPhrase& p2) {
  return ((p1[0] <= p2[0] && p2[0] <= p1[0] + p1[2]) || (p2[0] <= p1[0] && p1[0] <= p2[0] + p2[2]) ||
    (p1[1] <= p2[1] && p2[1] <= p1[1] + p1[3]) || (p2[1] <= p1[1] && p1[1] <= p2[1] + p2[3]));
}

MinPhrase combine(const MinPhrase& p1, const MinPhrase& p2) {
  MinPhrase c(4, 0);
  c[0] = std::min(p1[0], p2[0]);
  c[1] = std::min(p1[1], p2[1]);
  c[2] = std::max(p1[0] + p1[2] - c[0], p2[0] + p2[2] - c[0]);
  c[3] = std::max(p1[1] + p1[3] - c[1], p2[1] + p2[3] - c[1]);
  return c;
}

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>& source,
                      const std::vector<std::string>& target,
                      const std::vector<size_t>& alignment) {
  
  std::vector<bool> sourceAligned(source.size(), false);
  std::vector<bool> targetAligned(target.size(), false);
  
  MinPhrases minPhrases;
  for(size_t i = 0; i < alignment.size(); i += 2) {
    MinPhrase phrase(4, 0);
    phrase[0] = alignment[i];
    phrase[1] = alignment[i + 1];
    minPhrases.insert(phrase);
    
    sourceAligned[alignment[i]] = true;
    targetAligned[alignment[i + 1]] = true;
  }
  
  // @TODO: optimize this speed-wise
  repeat:
  BOOST_FOREACH(MinPhrase p1, minPhrases) {
    BOOST_FOREACH(MinPhrase p2, minPhrases) {
      if(p1 != p2) {
        if(overlap(p1, p2)) {
          minPhrases.erase(p1);
          minPhrases.erase(p2);   
          minPhrases.insert(combine(p1, p2));
          goto repeat;
        }
      }
    }
  }
  
  for(size_t i = 0; i < sourceAligned.size(); i++) {
    if(!sourceAligned[i]) {
      int start = i;
      int end = i;
      while(!sourceAligned[end] && end < (int)sourceAligned.size()) end++;
      
      MinPhrase sourcePhrase(4, -1);
      sourcePhrase[0] = start;
      sourcePhrase[2] = end - start - 1;
      
      minPhrases.insert(sourcePhrase);
      i = end - 1;
    }
  }
  
  for(size_t i = 0; i < targetAligned.size(); i++) {
    if(!targetAligned[i]) {
      int start = i;
      int end = i;
      while(!targetAligned[end] && end < (int)targetAligned.size()) end++;
      
      MinPhrase targetPhrase(4, -1);
      targetPhrase[1] = start;
      targetPhrase[3] = end - start - 1;
      
      minPhrases.insert(targetPhrase);
      i = end - 1;
    }
  }
  
  std::vector<std::string> edits;
  BOOST_FOREACH(MinPhrase p, minPhrases) {
    std::vector<std::string> sourceVec;
    if(p[0] != -1)
      for(int i = p[0]; i <= p[0] + p[2]; i++)
        sourceVec.push_back(source[i]);
    
    std::vector<std::string> targetVec;
    if(p[1] != -1)
      for(int i = p[1]; i <= p[1] + p[3]; i++)
        targetVec.push_back(target[i]);
    
    std::string source = boost::join(sourceVec, "^");
    std::string target = boost::join(targetVec, "^");
    
    std::stringstream ss;
    if(source.size() == 0) {
      ss << "+_" << target << "(" << p[0] << "," << p[1] << "," << p[2] << "," << p[3] << ")";
    }
    else if(target.size() == 0) {
      ss << "-_" << source << "(" << p[0] << "," << p[1] << "," << p[2] << "," << p[3] << ")";     
    }
    else if(source == target) {
      ss << "=_" << source << "(" << p[0] << "," << p[1] << "," << p[2] << "," << p[3] << ")";
    }
    else {
      ss << "~_" << source << "_" << target << "(" << p[0] << "," << p[1] << "," << p[2] << "," << p[3] << ")";
    }
    edits.push_back(ss.str());
  }
  
  return edits;
}

}