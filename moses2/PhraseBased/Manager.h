/*
 * Manager.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <queue>
#include <cstddef>
#include <string>
#include <deque>
#include "../ManagerBase.h"
#include "../Phrase.h"
#include "../TargetPhrase.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "../EstimatedScores.h"
#include "../legacy/Bitmaps.h"
#include "InputPaths.h"

namespace Moses2
{

class System;
class TranslationTask;
class PhraseImpl;
class TargetPhraseImpl;
class SearchNormal;
class Search;
class Hypothesis;
class Sentence;
class OutputCollector;

class Manager: public ManagerBase
{
public:
  Manager(System &sys, const TranslationTask &task, const std::string &inputStr,
          long translationId);

  virtual ~Manager();

  Bitmaps &GetBitmaps() {
    return *m_bitmaps;
  }

  const EstimatedScores &GetEstimatedScores() const {
    return *m_estimatedScores;
  }

  const InputPaths &GetInputPaths() const {
    return m_inputPaths;
  }

  const TargetPhraseImpl &GetInitPhrase() const {
    return *m_initPhrase;
  }

  void Decode();
  std::string OutputBest() const;
  std::string OutputNBest();
  std::string OutputTransOpt();

protected:

  InputPaths m_inputPaths;
  Bitmaps *m_bitmaps;
  EstimatedScores *m_estimatedScores;
  TargetPhraseImpl *m_initPhrase;

  Search *m_search;

  // must be run in same thread as Decode()
  void Init();
  void CalcFutureScore();

};

}

