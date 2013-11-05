
#pragma once

#include "Phrase.h"
#include "Scores.h"

class TargetPhrase: public Phrase
{
public:
  static TargetPhrase *CreateFromString(const FeatureFunction &ff,
                                        const std::string &targetStr,
                                        const std::string &scoreStr,
                                        bool logScores);

  TargetPhrase(size_t size);

  TargetPhrase(const TargetPhrase &copy); // do not implement

  virtual ~TargetPhrase();

  Scores &GetScores() {
    return m_scores;
  }
  const Scores &GetScores() const {
    return m_scores;
  }

  virtual std::string Debug() const;

protected:
  Scores m_scores;
};

