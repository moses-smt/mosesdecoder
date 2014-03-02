#include <sstream>
#include "CorrectionPattern.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
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


namespace Moses
{

using namespace std;

typedef char Diff;
typedef std::vector<Diff> Diffs;

template <class Sequence, class Pred>
void CreateDiffRec(size_t** c,
              const Sequence &s1,
              const Sequence &s2,
              size_t start,
              size_t i,
              size_t j,
              Diffs& diffs,
              Pred pred) {
  if(i > 0 && j > 0 && pred(s1[i - 1 + start], s2[j - 1 + start])) {
    CreateDiffRec(c, s1, s2, start, i - 1, j - 1, diffs, pred);
    diffs.push_back(Diff('m'));
  }
  else if(j > 0 && (i == 0 || c[i][j-1] >= c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i, j-1, diffs, pred);
    diffs.push_back(Diff('i'));
  }
  else if(i > 0 && (j == 0 || c[i][j-1] < c[i-1][j])) {
    CreateDiffRec(c, s1, s2, start, i-1, j, diffs, pred);
    diffs.push_back(Diff('d'));
  }
}

template <class Sequence, class Pred>
Diffs CreateDiff(const Sequence& s1,
           const Sequence& s2,
           Pred pred) {
  
  Diffs diffs;
  
  size_t n = s2.size();
  
  int start = 0;
  int m_end = s1.size() - 1;
  int n_end = s2.size() - 1;
    
  while(start <= m_end && start <= n_end && pred(s1[start], s2[start])) {
    diffs.push_back(Diff('m'));
    start++;
  }
  while(start <= m_end && start <= n_end && pred(s1[m_end], s2[n_end])) {
    m_end--;
    n_end--;
  }
  
  size_t m_new = m_end - start + 1;
  size_t n_new = n_end - start + 1;
  
  size_t** c = new size_t*[m_new + 1];
  for(size_t i = 0; i <= m_new; ++i) {
    c[i] = new size_t[n_new + 1];
    c[i][0] = 0;
  }
  for(size_t j = 0; j <= n_new; ++j)
    c[0][j] = 0;  
  for(size_t i = 1; i <= m_new; ++i)
    for(size_t j = 1; j <= n_new; ++j)
      if(pred(s1[i - 1 + start], s2[j - 1 + start]))
        c[i][j] = c[i-1][j-1] + 1;
      else
        c[i][j] = c[i][j-1] > c[i-1][j] ? c[i][j-1] : c[i-1][j];
  
  CreateDiffRec(c, s1, s2, start, m_new, n_new, diffs, pred);
  
  for(size_t i = 0; i <= m_new; ++i)
    delete[] c[i];
  delete[] c;
    
  for (size_t i = n_end + 1; i < n; ++i)
    diffs.push_back(Diff('m'));
  
  return diffs;
}

template <class Sequence>
Diffs CreateDiff(const Sequence& s1, const Sequence& s2) {
  return CreateDiff(s1, s2, std::equal_to<typename Sequence::value_type>());
}

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
          std::string temp;
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
        std::string temp;
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
      std::string temp;
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

std::vector<std::string> CorrectionPattern::CreatePattern(const Tokens &s1, const Tokens &s2) const {
  Diffs diffs = CreateDiff(s1, s2);
  size_t i = 0, j = 0;
  char lastType = 'm';
  
  std::vector<std::string> patternList;
  
  Tokens source, target;
  BOOST_FOREACH(Diff type, diffs) {
    if(type == 'm') {
      if(lastType != 'm')
        patternList.push_back(CreateSinglePattern(source, target));
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
  if(lastType != 'm')
    patternList.push_back(CreateSinglePattern(source, target));
    
  return patternList;
}

CorrectionPattern::CorrectionPattern(const std::string &line)
  :StatelessFeatureFunction(0, line),
   m_unrestricted(true)
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
  }

  inFile.close();

  m_unrestricted = false;
}

void CorrectionPattern::Evaluate(const Phrase &source
    , const TargetPhrase &target
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection &estimatedFutureScore) const
{
  ComputeFeatures(source, target, &scoreBreakdown);
}

void CorrectionPattern::ComputeFeatures(
    const Phrase &source,
    const TargetPhrase& target,
    ScoreComponentCollection* accumulator) const
{
  std::vector<std::string> sourceTokens;
  for(size_t i = 0; i < source.GetSize(); ++i)
    sourceTokens.push_back(source.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
  
  std::vector<std::string> targetTokens;
  for(size_t i = 0; i < target.GetSize(); ++i)
    targetTokens.push_back(target.GetWord(i).GetFactor(m_factorType)->GetString().as_string());
  
  std::vector<std::string> patternList = CreatePattern(sourceTokens, targetTokens);
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
