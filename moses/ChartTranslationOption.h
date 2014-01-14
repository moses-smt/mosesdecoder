#pragma once

#include "ScoreComponentCollection.h"

namespace Moses
{
class TargetPhrase;
class InputPath;
class InputType;

class ChartTranslationOption
{
protected:
  const TargetPhrase &m_targetPhrase;
  ScoreComponentCollection m_scoreBreakdown;

public:
  ChartTranslationOption(const TargetPhrase &targetPhrase);

  const TargetPhrase &GetPhrase() const {
    return m_targetPhrase;
  }

  const ScoreComponentCollection &GetScores() const {
    return m_scoreBreakdown;
  }

  void Evaluate(const InputType &input, const InputPath &inputPath);
};

}

