/*
 * MSPT.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#pragma once

#include "../PhraseTable.h"
#include "../../legacy/Util2.h"
#include "../../SCFG/InputPath.h"
#include "DynamicPhraseTableNode.h"
#include "../../PhraseBased/PhraseImpl.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../PhraseBased/TargetPhrases.h"
#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"
#include "../../SCFG/TargetPhrases.h"

namespace Moses2
{

class DynamicPhraseTable: public PhraseTable
{
  typedef DynamicPhraseTableNS::Node<Word, Phrase<Word>, TargetPhraseImpl, TargetPhrases> PBNODE;
  typedef DynamicPhraseTableNS::Node<SCFG::Word, Phrase<SCFG::Word>, SCFG::TargetPhraseImpl, SCFG::TargetPhrases> SCFGNODE;

//////////////////////////////////////
  class ActiveChartEntryMem : public SCFG::ActiveChartEntry
  {
    typedef SCFG::ActiveChartEntry Parent;
  public:
    const DynamicPhraseTable::SCFGNODE &node;

    ActiveChartEntryMem(MemPool &pool, const DynamicPhraseTable::SCFGNODE &vnode)
      :Parent(pool)
      ,node(vnode)
    {}

    ActiveChartEntryMem(
      MemPool &pool,
      const DynamicPhraseTable::SCFGNODE &vnode,
      const ActiveChartEntry &prevEntry)
      :Parent(prevEntry)
      ,node(vnode)
    {}
  };

  //////////////////////////////////////
public:
  DynamicPhraseTable(size_t startInd, const std::string &line);
  virtual ~DynamicPhraseTable();

  virtual TargetPhrases *Lookup(const Manager &mgr, MemPool &pool,
                                InputPath &inputPath) const;

  virtual void InitActiveChart(
    MemPool &pool,
    const SCFG::Manager &mgr,
    SCFG::InputPath &path) const;

  void Lookup(MemPool &pool,
              const SCFG::Manager &mgr,
              size_t maxChartSpan,
              const SCFG::Stacks &stacks,
              SCFG::InputPath &path) const;

  virtual void InitializeForInput(const ManagerBase &mgr, const InputType &input);
  virtual void CleanUpAfterSentenceProcessing(const System &system, const InputType &input) const;

protected:
  thread_local static PBNODE m_rootPb;

  void LookupGivenNode(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::ActiveChartEntry &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::Hypotheses *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const;

  void CreatePTForInput(const ManagerBase &mgr, std::string phraseTableString);

};

}

