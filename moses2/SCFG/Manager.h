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
#include "Stacks.h"
#include "InputPaths.h"
#include "Misc.h"

namespace Moses2
{

namespace SCFG
{
class SymbolBind;
class TargetPhraseImpl;
class SymbolBindElement;

class Manager: public Moses2::ManagerBase
{
public:
  Manager(System &sys, const TranslationTask &task, const std::string &inputStr,
          long translationId);

  virtual ~Manager();
  void Decode();
  std::string OutputBest() const;
  std::string OutputNBest();
  std::string OutputTransOpt();

  const InputPaths &GetInputPaths() const {
    return m_inputPaths;
  }

  QueueItemRecycler &GetQueueItemRecycler() {
    return m_queueItemRecycler;
  }

  const Stacks &GetStacks() const {
    return m_stacks;
  }

protected:
  Stacks m_stacks;
  SCFG::InputPaths m_inputPaths;

  void InitActiveChart(SCFG::InputPath &path);
  void Lookup(SCFG::InputPath &path);
  void LookupUnary(SCFG::InputPath &path);
  void Decode(SCFG::InputPath &path, Stack &stack);

  void ExpandHypo(
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind,
    const SCFG::TargetPhraseImpl &tp,
    Stack &stack);

  bool IncrPrevHypoIndices(
    Vector<size_t> &prevHyposIndices,
    size_t ind,
    const std::vector<const SymbolBindElement*> ntEles);

  // cube pruning
  Queue m_queue;
  SeenPositions m_seenPositions;

  QueueItemRecycler m_queueItemRecycler;

  void CreateQueue(
    const SCFG::InputPath &path,
    const SymbolBind &symbolBind,
    const SCFG::TargetPhrases &tps);
};

}
}

