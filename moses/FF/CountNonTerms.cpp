#include "CountNonTerms.h"
#include "moses/Util.h"
#include "moses/TargetPhrase.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{
CountNonTerms::CountNonTerms(const std::string &line)
  :StatelessFeatureFunction(line)
  ,m_all(true)
  ,m_sourceSyntax(false)
  ,m_targetSyntax(false)
{
  ReadParameters();
}

void CountNonTerms::EvaluateInIsolation(const Phrase &sourcePhrase
                                        , const TargetPhrase &targetPhrase
                                        , ScoreComponentCollection &scoreBreakdown
                                        , ScoreComponentCollection &estimatedFutureScore) const
{
  const StaticData &staticData = StaticData::Instance();

  vector<float> scores(m_numScoreComponents, 0);
  size_t indScore = 0;

  if (m_all) {
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      const Word &word = targetPhrase.GetWord(i);
      if (word.IsNonTerminal()) {
        ++scores[indScore];
      }
    }
    ++indScore;
  }

  if (m_targetSyntax) {
    for (size_t i = 0; i < targetPhrase.GetSize(); ++i) {
      const Word &word = targetPhrase.GetWord(i);
      if (word.IsNonTerminal() && word != staticData.GetOutputDefaultNonTerminal()) {
        ++scores[indScore];
      }
    }
    ++indScore;
  }

  if (m_sourceSyntax) {
    for (size_t i = 0; i < sourcePhrase.GetSize(); ++i) {
      const Word &word = sourcePhrase.GetWord(i);
      if (word.IsNonTerminal() && word != staticData.GetInputDefaultNonTerminal()) {
        ++scores[indScore];
      }
    }
    ++indScore;
  }

  scoreBreakdown.PlusEquals(this, scores);
}

void CountNonTerms::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "all") {
    m_all = Scan<bool>(value);
  } else if (key == "source-syntax") {
    m_sourceSyntax = Scan<bool>(value);
  } else if (key == "target-syntax") {
    m_targetSyntax = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


}
