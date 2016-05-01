/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#pragma once

#include "../PhraseTable.h"
#include "../../legacy/Util2.h"
#include "../../SCFG/InputPath.h"
#include "Node.h"
#include "../../PhraseBased/PhraseImpl.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../PhraseBased/TargetPhrases.h"
#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"
#include "../../SCFG/TargetPhrases.h"

namespace Moses2
{

class PhraseTableMemory: public PhraseTable
{
  typedef PtMem::Node<Word, Phrase<Word>, TargetPhraseImpl, TargetPhrases> PBNODE;
  typedef PtMem::Node<SCFG::Word, Phrase<SCFG::Word>, SCFG::TargetPhraseImpl, SCFG::TargetPhrases> SCFGNODE;

//////////////////////////////////////
  class ActiveChartEntryMem : public SCFG::ActiveChartEntry
  {
  public:
    const PhraseTableMemory::SCFGNODE &node;

    ActiveChartEntryMem(const PhraseTableMemory::SCFGNODE &vnode)
    :node(vnode)
    {}

    ActiveChartEntryMem(
        const SCFG::InputPath &subPhrasePath,
        const SCFG::Word &word,
        const PhraseTableMemory::SCFGNODE &vnode,
        const ActiveChartEntry &prevEntry)
    :ActiveChartEntry(subPhrasePath, word, prevEntry)
    ,node(vnode)
    {}
  };

  //////////////////////////////////////
public:
  PhraseTableMemory(size_t startInd, const std::string &line);
  virtual ~PhraseTableMemory();

  virtual void Load(System &system);
  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
      InputPath &inputPath) const;

  virtual void InitActiveChart(SCFG::InputPath &path) const;
  void Lookup(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

  void LookupUnary(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  PBNODE    *m_rootPb;
  SCFGNODE  *m_rootSCFG;

  void LookupGivenPath(
      SCFG::InputPath &path,
      const SCFG::InputPath &prevPath,
      const SCFG::Word &wordSought,
      const SCFG::InputPath &subPhrasePath) const;
  void LookupGivenNode(
      const ActiveChartEntryMem &prevEntry,
      const SCFG::Word &wordSought,
      const SCFG::InputPath &subPhrasePath,
      SCFG::InputPath &path) const;

  void LookupNT(
      SCFG::InputPath &path,
      const SCFG::InputPath &subPhrasePath,
      const SCFG::InputPath &prevPath,
      const SCFG::Stacks &stacks) const;


  void AddTargetPhrasesToPath(const SCFGNODE &node,
      const SCFG::SymbolBind &symbolBind,
      SCFG::InputPath &path) const;
};

}

