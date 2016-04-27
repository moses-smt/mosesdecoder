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
#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"

namespace Moses2
{

class PhraseTableMemory: public PhraseTable
{
  typedef PtMem::Node<Word, PhraseImpl, TargetPhraseImpl> PBNODE;
  typedef PtMem::Node<SCFG::Word, SCFG::PhraseImpl, SCFG::TargetPhraseImpl> SCFGNODE;

//////////////////////////////////////
  class ActiveChartEntryMem : public SCFG::ActiveChartEntry
  {
  public:
    const PhraseTableMemory::SCFGNODE *node;

    ActiveChartEntryMem(const SCFG::InputPath *subPhrasePath, bool isNT, const PhraseTableMemory::SCFGNODE *vnode)
    :ActiveChartEntry(subPhrasePath, isNT)
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

protected:
  PBNODE    m_rootPb;
  SCFGNODE  m_rootSCFG;

  void LookupGivenPrefixPath(const SCFG::InputPath &prefixPath,
      const SCFG::Word &wordSought,
      const SCFG::InputPath &subPhrasePath,
      bool isNT,
      SCFG::InputPath &path) const;
  void LookupGivenNode(const SCFGNODE  &node,
      const SCFG::Word &wordSought,
      const SCFG::InputPath &subPhrasePath,
      bool isNT,
      SCFG::InputPath &path) const;
  void AddTargetPhrasesToPath(const SCFGNODE &node,
      const SCFG::SymbolBind &symbolBind,
      SCFG::InputPath &path) const;
};

}

