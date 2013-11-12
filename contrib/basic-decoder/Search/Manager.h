
#pragma once

#include "Sentence.h"
#include "Stacks.h"
#include "TargetPhrase.h"
#include "WordsRange.h"

class InputPath;

class Manager
{
public:
  Manager(Sentence &sentence);
  virtual ~Manager();

  const Hypothesis *GetHypothesis() const;

  std::string Debug() const;

protected:
  Sentence &m_sentence;
  std::vector<InputPath*> m_inputPathQueue;
  Stacks m_stacks;
  
  TargetPhrase *m_emptyPhrase;
  WordsRange *m_emptyRange;
  WordsBitmap *m_emptyCoverage;

  void CreateInputPaths();
  void CreateInputPaths(const InputPath &prevPath, size_t pos);

  void Lookup();
  void Search();
};

