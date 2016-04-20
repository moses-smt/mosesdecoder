/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */
#pragma once

#include "PhraseTable.h"
#include "../legacy/Util2.h"
#include "../SCFG/InputPath.h"

namespace Moses2
{

class PhraseTableMemory: public PhraseTable
{
//////////////////////////////////////
  class Node
  {
  public:

    Node();
    ~Node();
    void AddRule(Phrase &source, TargetPhrase *target);
    TargetPhrases *Find(const Phrase &source, size_t pos = 0) const;
    const Node *Find(const Word &word) const;

    const TargetPhrases *GetTargetPhrases() const
    { return m_targetPhrases; }

    void SortAndPrune(size_t tableLimit, MemPool &pool, System &system);

  protected:
    typedef boost::unordered_map<Word, Node, UnorderedComparer<Word>,
        UnorderedComparer<Word> > Children;
    Children m_children;
    TargetPhrases *m_targetPhrases;
    Phrase *m_source;
    std::vector<TargetPhrase*> *m_unsortedTPS;

    Node &AddRule(Phrase &source, TargetPhrase *target, size_t pos);

  };
//////////////////////////////////////
  class ActiveChartEntryMem : public SCFG::ActiveChartEntry
  {
  public:
    const Node *node;

    ActiveChartEntryMem(const SCFG::InputPath *subPhrasePath, bool isNT, const Node *vnode)
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
      InputPathBase &inputPath) const;

  virtual void InitActiveChart(SCFG::InputPath &path) const;
  void Lookup(MemPool &pool,
      const SCFG::Manager &mgr,
      const SCFG::Stacks &stacks,
      SCFG::InputPath &path) const;

protected:
  Node m_root;

  void LookupGivenPrefixPath(const SCFG::InputPath &prefixPath,
      const Word &wordSought,
      const SCFG::InputPath &subPhrasePath,
      bool isNT,
      SCFG::InputPath &path) const;
  void LookupGivenNode(const Node &node,
      const Word &wordSought,
      const SCFG::InputPath &subPhrasePath,
      bool isNT,
      SCFG::InputPath &path) const;
  void AddTargetPhrasesToPath(const Node &node,
      const SCFG::SymbolBind &symbolBind,
      SCFG::InputPath &path) const;
};

}

