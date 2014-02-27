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

typedef std::pair<size_t, char> Diff;
typedef std::vector<Diff> Diffs;

template <class Sequence, class Pred>
void diff_rec(size_t** c,
              Sequence s1,
              Sequence s2,
              size_t start,
              size_t i,
              size_t j,
              Diffs& diffs,
              Pred pred) {
  if(i > 0 && j > 0 && pred(s1[i - 1 + start], s2[j - 1 + start])) {
    diff_rec(c, s1, s2, start, i - 1, j - 1, diffs, pred);
    diffs.push_back(Diff(j - 1 + start, 'm'));
  }
  else if(j > 0 && (i == 0 || c[i][j-1] >= c[i-1][j])) {
    diff_rec(c, s1, s2, start, i, j-1, diffs, pred);
    diffs.push_back(Diff(j - 1 + start, 'i'));
  }
  else if(i > 0 && (j == 0 || c[i][j-1] < c[i-1][j])) {
    diff_rec(c, s1, s2, start, i-1, j, diffs, pred);
    diffs.push_back(Diff(i - 1 + start, 'd'));
  }
}

template <class Sequence, class Pred>
Diffs diff(const Sequence& s1,
           const Sequence& s2,
           Pred pred) {
  
  Diffs diffs;
  
  size_t n = s2.size();
  
  int start = 0;
  int m_end = s1.size() - 1;
  int n_end = s2.size() - 1;
    
  while(start <= m_end && start <= n_end && pred(s1[start], s2[start])) {
    diffs.push_back(Diff(start, 'm'));
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
  
  //size_t length = c[m_new][n_new] + (m - m_end - 1) + start;
  
  diff_rec(c, s1, s2, start, m_new, n_new, diffs, pred);
  
  for(size_t i = 0; i <= m_new; ++i)
    delete[] c[i];
  delete[] c;
    
  for (size_t i = n_end + 1; i < n; ++i)
    diffs.push_back(Diff(i, 'm'));
  
  return diffs;
}

template <class Sequence>
Diffs diff(const Sequence& s1,
          const Sequence& s2) {
  return diff(s1, s2, std::equal_to<typename Sequence::value_type>());
}

template <class Sequence>
std::string make_pattern(const Sequence &s1, const Sequence &s2) {
  typedef typename Sequence::value_type Item;
  
  std::stringstream out;
  if(s1.empty())
    out << "ins(«" << boost::join(s2, "·") << "»)";
  else if(s2.empty())
    out << "del(«" << boost::join(s1, "·") << "»)";
  else
    out << "sub(«" << boost::join(s1, "·") << "»,«"
      << boost::join(s2, "·") << "»)";
  return out.str();
}

template <class Sequence>
std::vector<std::string> pattern(const Sequence &s1, const Sequence &s2) {
  Diffs diffs = diff(s1, s2);
  size_t i = 0, j = 0;
  char lastType = 'm';
  
  std::vector<std::string> patternList;
  
  Sequence source, target;
  BOOST_FOREACH(Diff item, diffs) {
    char type = item.second;
    if(type == 'm') {
      if(lastType != 'm')
        patternList.push_back(make_pattern(source, target));
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
    patternList.push_back(make_pattern(source, target));
    
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
  
  std::vector<std::string> patternList = pattern(sourceTokens, targetTokens);
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
