#include <sstream>
#include "CorrectionPattern.h"
#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/InputPath.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TranslationOption.h"
#include "util/string_piece_hash.hh"
#include "util/exception.hh"

#include <functional>
#include <algorithm>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include "Diffs.h"

namespace Moses
{

using namespace std;

std::string MakePair(const std::string &s1, const std::string &s2, bool general)
{
  std::vector<std::string> sourceList;
  std::vector<std::string> targetList;

  if(general) {
    Diffs diffs = CreateDiff(s1, s2);

    size_t i = 0, j = 0;
    char lastType = 'm';

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
          } else {
            sourceList.push_back(match + "·");
            targetList.push_back(match + "·");
          }
          match.clear();
        } else
          match.push_back(s1[i]);

        i++;
        j++;
      } else if(type == 'd') {
        if(s1[i] == '+')
          source += "·";
        else
          source.push_back(s1[i]);
        i++;
      } else if(type == 'i') {
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
        } else {
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
      } else {
        sourceList.push_back(match);
        targetList.push_back(match);
      }
    }
    match.clear();
  } else {
    std::string cs1 = s1;
    std::string cs2 = s2;
    boost::replace_all(cs1, "+", "·");
    boost::replace_all(cs2, "+", "·");

    sourceList.push_back(cs1);
    targetList.push_back(cs2);
  }

  std::stringstream out;
  out << "sub(«";
  out << boost::join(sourceList, "");
  out << "»,«";
  out << boost::join(targetList, "");
  out << "»)";

  return out.str();
}

std::string CorrectionPattern::CreateSinglePattern(const Tokens &s1, const Tokens &s2) const
{
  std::stringstream out;
  if(s1.empty()) {
    out << "ins(«" << boost::join(s2, "·") << "»)";
    return out.str();
  } else if(s2.empty()) {
    out << "del(«" << boost::join(s1, "·") << "»)";
    return out.str();
  } else {
    Tokens::value_type v1 = boost::join(s1, "+");
    Tokens::value_type v2 = boost::join(s2, "+");
    out << MakePair(v1, v2, m_general);
    return out.str();
  }
}

std::vector<std::string> GetContext(size_t pos,
                                    size_t len,
                                    size_t window,
                                    const InputType &input,
                                    const InputPath &inputPath,
                                    const std::vector<FactorType>& factorTypes,
                                    bool isRight)
{

  const Sentence& sentence = static_cast<const Sentence&>(input);
  const Range& range = inputPath.GetWordsRange();

  int leftPos  = range.GetStartPos() + pos - len - 1;
  int rightPos = range.GetStartPos() + pos;

  std::vector<std::string> contexts;

  for(int length = 1; length <= (int)window; ++length) {
    std::vector<std::string> current;
    if(!isRight) {
      for(int i = 0; i < length; i++) {
        if(leftPos - i >= 0) {
          current.push_back(sentence.GetWord(leftPos - i).GetString(factorTypes, false));
        } else {
          current.push_back("<s>");
        }
      }

      if(current.back() == "<s>" && current.size() >= 2 && current[current.size()-2] == "<s>")
        continue;

      std::reverse(current.begin(), current.end());
      contexts.push_back("left(«" + boost::join(current, "·") + "»)_");
    }
    if(isRight) {
      for(int i = 0; i < length; i++) {
        if(rightPos + i < (int)sentence.GetSize()) {
          current.push_back(sentence.GetWord(rightPos + i).GetString(factorTypes, false));
        } else {
          current.push_back("</s>");
        }
      }

      if(current.back() == "</s>" && current.size() >= 2 && current[current.size()-2] == "</s>")
        continue;

      contexts.push_back("_right(«" + boost::join(current, "·") + "»)");
    }
  }
  return contexts;
}

std::vector<std::string>
CorrectionPattern::CreatePattern(const Tokens &s1,
                                 const Tokens &s2,
                                 const InputType &input,
                                 const InputPath &inputPath) const
{

  Diffs diffs = CreateDiff(s1, s2);
  size_t i = 0, j = 0;
  char lastType = 'm';
  std::vector<std::string> patternList;
  Tokens source, target;
  BOOST_FOREACH(Diff type, diffs) {
    if(type == 'm') {
      if(lastType != 'm') {
        std::string pattern = CreateSinglePattern(source, target);
        patternList.push_back(pattern);

        if(m_context > 0) {
          std::vector<std::string> leftContexts =  GetContext(i, source.size(), m_context, input, inputPath, m_contextFactors, false);
          std::vector<std::string> rightContexts = GetContext(i, source.size(), m_context, input, inputPath, m_contextFactors, true);

          BOOST_FOREACH(std::string left, leftContexts)
          patternList.push_back(left + pattern);

          BOOST_FOREACH(std::string right, rightContexts)
          patternList.push_back(pattern + right);

          BOOST_FOREACH(std::string left, leftContexts)
          BOOST_FOREACH(std::string right, rightContexts)
          patternList.push_back(left + pattern + right);
        }
      }
      source.clear();
      target.clear();
      if(s1[i] != s2[j]) {
        source.push_back(s1[i]);
        target.push_back(s2[j]);
      }
      i++;
      j++;
    } else if(type == 'd') {
      source.push_back(s1[i]);
      i++;
    } else if(type == 'i') {
      target.push_back(s2[j]);
      j++;
    }
    lastType = type;
  }
  if(lastType != 'm') {
    std::string pattern = CreateSinglePattern(source, target);
    patternList.push_back(pattern);

    if(m_context > 0) {
      std::vector<std::string> leftContexts =  GetContext(i, source.size(), m_context, input, inputPath, m_contextFactors, false);
      std::vector<std::string> rightContexts = GetContext(i, source.size(), m_context, input, inputPath, m_contextFactors, true);

      BOOST_FOREACH(std::string left, leftContexts)
      patternList.push_back(left + pattern);

      BOOST_FOREACH(std::string right, rightContexts)
      patternList.push_back(pattern + right);

      BOOST_FOREACH(std::string left, leftContexts)
      BOOST_FOREACH(std::string right, rightContexts)
      patternList.push_back(left + pattern + right);
    }
  }

  return patternList;
}

CorrectionPattern::CorrectionPattern(const std::string &line)
  : StatelessFeatureFunction(0, line), m_factors(1, 0), m_general(false),
    m_context(0), m_contextFactors(1, 0)
{
  std::cerr << "Initializing correction pattern feature.." << std::endl;
  ReadParameters();
}

void CorrectionPattern::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factors = std::vector<FactorType>(1, Scan<FactorType>(value));
  } else if (key == "context-factor") {
    m_contextFactors = std::vector<FactorType>(1, Scan<FactorType>(value));
  } else if (key == "general") {
    m_general = Scan<bool>(value);
  } else if (key == "context") {
    m_context = Scan<size_t>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
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
    sourceTokens.push_back(source.GetWord(i).GetString(m_factors, false));

  std::vector<std::string> targetTokens;
  for(size_t i = 0; i < target.GetSize(); ++i)
    targetTokens.push_back(target.GetWord(i).GetString(m_factors, false));

  std::vector<std::string> patternList = CreatePattern(sourceTokens, targetTokens, input, inputPath);
  for(size_t i = 0; i < patternList.size(); ++i)
    accumulator->PlusEquals(this, patternList[i], 1);

  /*
  BOOST_FOREACH(std::string w, sourceTokens)
    std::cerr << w << " ";
  std::cerr << std::endl;
  BOOST_FOREACH(std::string w, targetTokens)
    std::cerr << w << " ";
  std::cerr << std::endl;
  BOOST_FOREACH(std::string w, patternList)
    std::cerr << w << " ";
  std::cerr << std::endl << std::endl;
  */
}

bool CorrectionPattern::IsUseable(const FactorMask &mask) const
{
  bool ret = true;
  for(size_t i = 0; i < m_factors.size(); ++i)
    ret = ret && mask[m_factors[i]];
  for(size_t i = 0; i < m_contextFactors.size(); ++i)
    ret = ret && mask[m_contextFactors[i]];
  return ret;
}

}
