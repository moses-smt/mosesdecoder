#include <sstream>
#include "CorrectionPattern.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/InputPath.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationOption.h"
#include "moses/UserMessage.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

#include <functional>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "Diffs.h"

namespace Moses
{

using namespace std;

std::string GeneralizePair(const std::string &s1, const std::string &s2) {
  Diffs diffs = CreateDiff(s1, s2);
  
  size_t i = 0, j = 0;
  char lastType = 'm';
  
  std::vector<std::string> sourceList;
  std::vector<std::string> targetList;
  
  std::string source, target;  
  std::string match;
  
  int count = 1;
  
  BOOST_FOREACH(Diff type, diffs) {
    if(type == 'm') {
      if(lastType != 'm') {
        sourceList.push_back(source);
        targetList.push_back(target);
      }
      source.clear();
      target.clear();
      
      if(s1[i] == '+') {
        if(match.size() >= 3) {
          sourceList.push_back("(\\w{3,})·");
          std::string temp = "1";
          sprintf((char*)temp.c_str(), "%d", count);
          targetList.push_back("\\" + temp + "·");
          count++;
        }
        else {
          sourceList.push_back(match + "·");
          targetList.push_back(match + "·");  
        }
        match.clear();
      }
      else 
        match.push_back(s1[i]);
      
      i++;
      j++;
    }
    else if(type == 'd') {
      if(s1[i] == '+')
        source += "·";
      else
        source.push_back(s1[i]);
      i++;
    }
    else if(type == 'i') {
      if(s2[j] == '+')
        target += "·";
      else
        target.push_back(s2[j]);
      j++;
    }
    if(type != 'm' && !match.empty()) {      
      if(match.size() >= 3) {
        sourceList.push_back("(\\w{3,})");
        std::string temp = "1";
        sprintf((char*)temp.c_str(), "%d", count);
        targetList.push_back("\\" + temp);
        count++;
      }
      else {
        sourceList.push_back(match);
        targetList.push_back(match);  
      }
    
      match.clear();
    }

    lastType = type;
  }
  if(lastType != 'm') {
    sourceList.push_back(source);
    targetList.push_back(target);
  }
  
  if(!match.empty()) {
    if(match.size() >= 3) {
      sourceList.push_back("(\\w{3,})");
      std::string temp = "1";
      sprintf((char*)temp.c_str(), "%d", count);
      targetList.push_back("\\"+ temp);
      count++;
    }
    else {
      sourceList.push_back(match);
      targetList.push_back(match);  
    }
  }
  match.clear();
  
  std::stringstream out;
  out << "sub(«";
  out << boost::join(sourceList, "");
  out << "»,«";
  out << boost::join(targetList, "");
  out << "»)";
  
  return out.str();
}

std::string CorrectionPattern::CreateSinglePattern(const Tokens &s1, const Tokens &s2) const {
  typedef typename Tokens::value_type Item;
  
  std::stringstream out;
  if(s1.empty()) {
    out << "ins(«" << boost::join(s2, "·") << "»)";
    if(m_unrestricted || m_vocab.count(out.str()))
      return out.str();
    else
      return "ins(OTHER)";
  }
  else if(s2.empty()) {
    out << "del(«" << boost::join(s1, "·") << "»)";
    if(m_unrestricted || m_vocab.count(out.str()))
      return out.str();
    else
      return "del(OTHER)";
  }
  else {
    typename Tokens::value_type v1 = boost::join(s1, "+");
    typename Tokens::value_type v2 = boost::join(s2, "+");
    out << GeneralizePair(v1, v2);
    if(m_unrestricted || m_vocab.count(out.str()))
      return out.str();
    else
      return "sub(OTHER1,OTHER2)";
  }
}

std::string GetContext(size_t pos,
                       size_t len,
                       size_t window,
                       const InputType &input,
                       const InputPath &inputPath,
                       FactorType factorType,
                       bool isRight) {

  const Sentence& sentence = static_cast<const Sentence&>(input);
  const WordsRange& range = inputPath.GetWordsRange(); 
  
  int leftPos  = range.GetStartPos() + pos - len - 1; 
  int rightPos = range.GetStartPos() + pos; 
  
  std::string context = isRight ? "_right(«</s>»)" : "left(«<s>»)_";
  
  if(!isRight && leftPos >= 0)
    context =
      "left(«"
      + sentence.GetWord(leftPos).GetFactor(factorType)->GetString().as_string()
      + "»)_";
    
  if(isRight && rightPos < sentence.GetSize())
    context =
      "_right(«"
      + sentence.GetWord(rightPos).GetFactor(factorType)->GetString().as_string()
      + "»)";
  
  return context;
}

std::vector<std::string>
CorrectionPattern::CreatePattern(const Tokens &s1,
                                 const Tokens &s2,
                                 const InputType &input,
                                 const InputPath &inputPath) const {
    
  Diffs diffs = CreateDiff(s1, s2);
  size_t i = 0, j = 0;
  char lastType = 'm';
  std::vector<std::string> patternList;
  Tokens source, target;
  BOOST_FOREACH(Diff type, diffs) {
    if(type == 'm') {
      if(lastType != 'm') {
        std::string leftContext =  GetContext(i, source.size(), 1, input, inputPath, m_factorType, false);
        std::string rightContext = GetContext(i, source.size(), 1, input, inputPath, m_factorType, true);
        
        std::string pattern = CreateSinglePattern(source, target);
        //pattern = leftContext + pattern + rightContext; 
        
        patternList.push_back(pattern);
        patternList.push_back(pattern + rightContext);
        patternList.push_back(leftContext + pattern);
        patternList.push_back(leftContext + pattern + rightContext);
      }
      source.clear();
      target.clear();
      if(s1[i] != s2[j]) {
        source.push_back(s1[i]);
        target.push_back(s2[j]);
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
    std::string leftContext =  GetContext(i, source.size(), 1, input, inputPath, m_factorType, false);
    std::string rightContext = GetContext(i, source.size(), 1, input, inputPath, m_factorType, true);
    
    std::string pattern = CreateSinglePattern(source, target);
    //pattern = leftContext + pattern + rightContext; 
    
    patternList.push_back(pattern);
    patternList.push_back(pattern + rightContext);
    patternList.push_back(leftContext + pattern);
    patternList.push_back(leftContext + pattern + rightContext);
  }
  
  return patternList;
}

CorrectionPattern::CorrectionPattern(const std::string &line)
  :StatelessFeatureFunction(0, line),
   m_unrestricted(true), m_top(0)
{
  std::cerr << "Initializing correction pattern feature.." << std::endl;
  ReadParameters();
}

void CorrectionPattern::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else if (key == "path") {
    m_filename = value;
  } else if (key == "top") {
    m_top = Scan<size_t>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void CorrectionPattern::Load()
{
  if (m_filename.empty())
    return;

  cerr << "loading pattern list from " << m_filename << endl;
  ifstream inFile(m_filename.c_str());
  UTIL_THROW_IF2(!inFile, "could not open file " << m_filename);

  std::string line;
  while (getline(inFile, line)) {
    m_vocab.insert(line);
    if(m_top && m_vocab.size() >= m_top)
      break;
  }

  inFile.close();

  m_unrestricted = false;
}

void CorrectionPattern::EvaluateWithSourceContext(const InputType &input
              , const InputPath &inputPath
              , const TargetPhrase &targetPhrase
              , const StackVec *stackVec
              , ScoreComponentCollection &scoreBreakdown
              , ScoreComponentCollection *estimatedFutureScore) const
{
  ComputeFeatures(input, inputPath, targetPhrase, &scoreBreakdown);
}

void CorrectionPattern::ComputeFeatures(
    const InputType &input,
    const InputPath &inputPath,
    const TargetPhrase& target,
    ScoreComponentCollection* accumulator) const
{
  const Phrase &source = inputPath.GetPhrase();

  std::vector<std::string> sourceTokens;
  for(size_t i = 0; i < source.GetSize(); ++i)
    sourceTokens.push_back(source.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
  
  std::vector<std::string> targetTokens;
  for(size_t i = 0; i < target.GetSize(); ++i)
    targetTokens.push_back(target.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
  
  std::vector<std::string> patternList = CreatePattern(sourceTokens, targetTokens, input, inputPath);
  for(size_t i = 0; i < patternList.size(); ++i)
    accumulator->PlusEquals(this, patternList[i], 1);
  
  //BOOST_FOREACH(std::string w, sourceTokens)
  //  std::cerr << w << " ";
  //std::cerr << std::endl;
  //BOOST_FOREACH(std::string w, targetTokens)
  //  std::cerr << w << " ";
  //std::cerr << std::endl;
  //BOOST_FOREACH(std::string w, patternList)
  //  std::cerr << w << " ";
  //std::cerr << std::endl << std::endl;
}

bool CorrectionPattern::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

}
