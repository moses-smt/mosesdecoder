#pragma once

#include "ScoreComponentCollection.h"

namespace Moses
{
class TargetPhrase;
class InputPath;
class InputType;
class StackVec;

class ChartTranslationOption
{
  friend std::ostream& operator<<(std::ostream&, const ChartTranslationOption&);

protected:
  const TargetPhrase &m_targetPhrase;
  ScoreComponentCollection m_scoreBreakdown;
  const InputPath *m_inputPath;
  const std::vector<const Word*> *m_ruleSourceFromInputPath; // used by placeholders

public:
  ChartTranslationOption(const TargetPhrase &targetPhrase);

  const TargetPhrase &GetPhrase() const {
    return m_targetPhrase;
  }

  const InputPath *GetInputPath() const {
    return m_inputPath;
  }

  void SetInputPath(const InputPath *inputPath) {
    m_inputPath = inputPath;
  }

  const std::vector<const Word*> *GetSourceRuleFromInputPath() const {
    return m_ruleSourceFromInputPath;
  }
  void SetSourceRuleFromInputPath(const std::vector<const Word*> *obj) {
    m_ruleSourceFromInputPath = obj;
  }

  const ScoreComponentCollection &GetScores() const {
    return m_scoreBreakdown;
  }

  void EvaluateWithSourceContext(const InputType &input,
		  const InputPath &inputPath,
		  const StackVec &stackVec);
};

}

