#include "UtilESM.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <sstream>

#include "moses/FF/Diffs.h"

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

typedef std::vector<std::string> Tokens;

std::vector<std::string> CreateSinglePattern(const Tokens &s1, const Tokens &s2) {
  std::vector<std::string> pattern;
  
  if(s1.empty()) {
    BOOST_FOREACH(std::string w, s2) {
     std::stringstream out;
     out << "+_" << w;
     pattern.push_back(out.str());
    }
  }
  else if(s2.empty()) {
    BOOST_FOREACH(std::string w, s2) {
     std::stringstream out;
     out << "-_" << w;
     pattern.push_back(out.str());
    }
  }
  else {
    for(size_t i = 0; i < std::max(s1.size(), s2.size()); i++) {
      std::stringstream out;
      if(i < s1.size() && i < s2.size()) {
        if(s1[i] == s2[i])
          out << "=_" << s1[i];
        else
          out << "~_" << s1[i] << "_" << s2[i];
      }
      else if(i < s1.size()) {
        out << "-_" << s1[i];   
      }
      else if(i < s2.size()) {
        out << "+_" << s2[i];   
      }
      pattern.push_back(out.str());
    }
  }
  
  return pattern;
}

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>& s1,
                      const std::vector<std::string>& s2) {
      
  Diffs diffs = CreateDiff(s1, s2);
  size_t i = 0, j = 0;
  char lastType = 'm';
  std::vector<std::string> patternList;
  Tokens source, target;
  BOOST_FOREACH(Diff type, diffs) {
    if(type == 'm') {
      if(lastType != 'm') {
        std::vector<std::string> patterns = CreateSinglePattern(source, target);
        patternList.insert(patternList.end(), patterns.begin(), patterns.end());
      }
      source.clear();
      target.clear();
      
      if(s1[i] != s2[j]) {
        source.push_back(s1[i]);
        target.push_back(s2[j]);
      }
      else {
        source.push_back(s1[i]);
        target.push_back(s2[j]);
        std::vector<std::string> patterns = CreateSinglePattern(source, target);
        patternList.insert(patternList.end(), patterns.begin(), patterns.end());
        source.clear();
        target.clear();
      }
      
      i++;
      j++;
    }
    else if(type == 'd') {
      source.push_back(s1[i]);
      i++;
    }
    else if(type == 'i') {
      target.push_back(s2[j]);
      j++;
    }
    lastType = type;
  }
  if(lastType != 'm') {    
    if(!source.empty() || !target.empty()) {
      std::vector<std::string> patterns = CreateSinglePattern(source, target);
      patternList.insert(patternList.end(), patterns.begin(), patterns.end());
    }
  }  
  
  return patternList;
}

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>& source,
                      const std::vector<std::string>& target,
                      const std::vector<size_t>& alignment) {

  std::vector<bool> sourceAligned(source.size(), false);
  std::vector<std::vector<size_t> > targetAligned(target.size());
  
  for(size_t i = 0; i < alignment.size(); i += 2) {    
    sourceAligned[alignment[i]] = true;
    targetAligned[alignment[i + 1]].push_back(alignment[i]);
  }
  
  std::vector<std::string> edits;
  for(size_t i = 0; i < targetAligned.size(); i++) {
    std::stringstream pattern;
    
    if(targetAligned[i].empty()) {
      pattern << "+_" << target[i];
      edits.push_back(pattern.str());
    }
    else {
      std::stringstream sourceStream;
      sourceStream << source[targetAligned[i][0]];
      for(size_t j = 1; j < targetAligned[i].size(); j++)
        sourceStream << "^" << source[targetAligned[i][j]];
      std::string sourceString = sourceStream.str();
      if(sourceString == target[i])
        pattern << "=_" << sourceString;
      else
        pattern << "~_" << sourceString << "_" << target[i];
      edits.push_back(pattern.str());
        
      for(size_t j = 0; j < targetAligned[i].size(); j++) {
        size_t k = targetAligned[i][j] + 1;
        while(!sourceAligned[k] && k < source.size()) {
          std::stringstream dropStream;
          dropStream << "-_" << source[k];
          edits.push_back(dropStream.str());
          k++;
        }
      } 
    }
  }
  
  return edits;
}

/*
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
      ss << "+_" << target;
    }
    else if(target.size() == 0) {
      ss << "-_" << source;     
    }
    else if(source == target) {
      ss << "=_" << source;
    }
    else {
      ss << "~_" << source << "_" << target;
    }
    edits.push_back(ss.str());
  }
  
  return edits;
}
*/
}