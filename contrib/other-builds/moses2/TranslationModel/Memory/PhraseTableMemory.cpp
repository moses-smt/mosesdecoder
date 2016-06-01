/*
 * PhraseTableMemory.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <boost/foreach.hpp>
#include "PhraseTableMemory.h"
#include "../../PhraseBased/PhraseImpl.h"
#include "../../Phrase.h"
#include "../../System.h"
#include "../../Scores.h"
#include "../../InputPathsBase.h"
#include "../../legacy/InputFileStream.h"
#include "util/exception.hh"

#include "../../PhraseBased/InputPath.h"
#include "../../PhraseBased/TargetPhraseImpl.h"
#include "../../PhraseBased/TargetPhrases.h"

#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"
#include "../../SCFG/InputPath.h"
#include "../../SCFG/Stack.h"
#include "../../SCFG/Stacks.h"
#include "../../SCFG/Manager.h"


using namespace std;

namespace Moses2
{


////////////////////////////////////////////////////////////////////////

PhraseTableMemory::PhraseTableMemory(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
,m_rootPb(NULL)
,m_rootSCFG(NULL)
{
  ReadParameters();
}

PhraseTableMemory::~PhraseTableMemory()
{
  delete m_rootPb;
  delete m_rootSCFG;
}

void PhraseTableMemory::Load(System &system)
{
  FactorCollection &vocab = system.GetVocab();
  MemPool &systemPool = system.GetSystemPool();
  MemPool tmpSourcePool;

  if (system.isPb) {
    m_rootPb = new PBNODE();
  }
  else {
    m_rootSCFG = new SCFGNODE();
    //cerr << "m_rootSCFG=" << m_rootSCFG << endl;
  }

  vector<string> toks;
  size_t lineNum = 0;
  InputFileStream strme(m_path);
  string line;
  while (getline(strme, line)) {
    if (++lineNum % 1000000 == 0) {
      cerr << lineNum << " ";
    }
    toks.clear();
    TokenizeMultiCharSeparator(toks, line, "|||");
    UTIL_THROW_IF2(toks.size() < 3, "Wrong format");
    //cerr << "line=" << line << endl;
    //cerr << "system.isPb=" << system.isPb << endl;

    if (system.isPb) {
      PhraseImpl *source = PhraseImpl::CreateFromString(tmpSourcePool, vocab, system,
          toks[0]);
      //cerr << "created soure" << endl;
      TargetPhraseImpl *target = TargetPhraseImpl::CreateFromString(systemPool, *this, system,
          toks[1]);
      //cerr << "created target" << endl;
      target->GetScores().CreateFromString(toks[2], *this, system, true);
      //cerr << "created scores:" << *target << endl;

      // properties
      if (toks.size() == 7) {
        //target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
        //strcpy(target->properties, toks[6].c_str());
      }

      system.featureFunctions.EvaluateInIsolation(systemPool, system, *source,
          *target);
      //cerr << "EvaluateInIsolation:" << *target << endl;
      m_rootPb->AddRule(*source, target);

    }
    else {
      SCFG::PhraseImpl *source = SCFG::PhraseImpl::CreateFromString(tmpSourcePool, vocab, system,
          toks[0]);
      //cerr << "created source:" << *source << endl;
      SCFG::TargetPhraseImpl *target = SCFG::TargetPhraseImpl::CreateFromString(systemPool, *this,
          system, toks[1]);
      target->SetAlignmentInfo(toks[3]);
      //cerr << "created target " << *target << " source=" << *source << endl;

      target->GetScores().CreateFromString(toks[2], *this, system, true);
      //cerr << "created scores:" << *target << endl;

      // properties
      if (toks.size() == 7) {
        //target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
        //strcpy(target->properties, toks[6].c_str());
      }

      system.featureFunctions.EvaluateInIsolation(systemPool, system, *source,
          *target);
      //cerr << "EvaluateInIsolation:" << *target << endl;
      m_rootSCFG->AddRule(*source, target);
    }
  }

  if (system.isPb) {
    m_rootPb->SortAndPrune(m_tableLimit, systemPool, system);
    cerr << "root=" << &m_rootPb << endl;
  }
  else {
    m_rootSCFG->SortAndPrune(m_tableLimit, systemPool, system);
    cerr << "root=" << &m_rootPb << endl;
  }
  /*
  BOOST_FOREACH(const PtMem::Node<Word>::Children::value_type &valPair, m_rootPb.GetChildren()) {
    const Word &word = valPair.first;
    cerr << word << " ";
  }
  cerr << endl;
  */
}

TargetPhrases* PhraseTableMemory::Lookup(const Manager &mgr, MemPool &pool,
    InputPath &inputPath) const
{
  const SubPhrase<Moses2::Word> &phrase = inputPath.subPhrase;
  TargetPhrases *tps = m_rootPb->Find(phrase);
  return tps;
}

void PhraseTableMemory::InitActiveChart(MemPool &pool, SCFG::InputPath &path) const
{
  size_t ptInd = GetPtInd();
  ActiveChartEntryMem *chartEntry = new (pool.Allocate<ActiveChartEntryMem>()) ActiveChartEntryMem(pool, *m_rootSCFG);
  path.AddActiveChartEntry(ptInd, chartEntry);
  //cerr << "InitActiveChart=" << path << endl;
}

void PhraseTableMemory::Lookup(MemPool &pool,
    const SCFG::Manager &mgr,
    size_t maxChartSpan,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const
{
  if (path.range.GetNumWordsCovered() > maxChartSpan) {
    return;
  }

  size_t endPos = path.range.GetEndPos();

  const SCFG::InputPath *prevPath = static_cast<const SCFG::InputPath*>(path.prefixPath);
  UTIL_THROW_IF2(prevPath == NULL, "prefixPath == NULL");

  // TERMINAL
  const SCFG::Word &lastWord = path.subPhrase.Back();

  const SCFG::InputPath &subPhrasePath = *mgr.GetInputPaths().GetMatrix().GetValue(endPos, 1);

  //cerr << "BEFORE LookupGivenWord=" << *prevPath << endl;
  LookupGivenWord(pool, *prevPath, lastWord, NULL, subPhrasePath.range, path);
  //cerr << "AFTER LookupGivenWord=" << *prevPath << endl;

  // NON-TERMINAL
  //const SCFG::InputPath *prefixPath = static_cast<const SCFG::InputPath*>(path.prefixPath);
  while (prevPath) {
    const Range &prevRange = prevPath->range;
    //cerr << "prevRange=" << prevRange << endl;

    size_t startPos = prevRange.GetEndPos() + 1;
    size_t ntSize = endPos - startPos + 1;
    const SCFG::InputPath &subPhrasePath = *mgr.GetInputPaths().GetMatrix().GetValue(startPos, ntSize);

    LookupNT(pool, subPhrasePath.range, *prevPath, stacks, path);

    prevPath = static_cast<const SCFG::InputPath*>(prevPath->prefixPath);
  }
}

void PhraseTableMemory::LookupUnary(
    MemPool &pool,
    const SCFG::Manager &mgr,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &path) const
{
  //cerr << "LookupUnary" << endl;

  size_t startPos = path.range.GetStartPos();
  const SCFG::InputPath *prevPath = mgr.GetInputPaths().GetMatrix().GetValue(startPos, 0);
  LookupNT(pool, path.range, *prevPath, stacks, path);
}

void PhraseTableMemory::LookupNT(
    MemPool &pool,
    const Moses2::Range &subPhraseRange,
    const SCFG::InputPath &prevPath,
    const SCFG::Stacks &stacks,
    SCFG::InputPath &outPath) const
{
  size_t endPos = outPath.range.GetEndPos();

  const Range &prevRange = prevPath.range;

  size_t startPos = prevRange.GetEndPos() + 1;
  size_t ntSize = endPos - startPos + 1;

  const SCFG::Stack &ntStack = stacks.GetStack(startPos, ntSize);
  const SCFG::Stack::Coll &stackColl = ntStack.GetColl();

  BOOST_FOREACH (const SCFG::Stack::Coll::value_type &valPair, stackColl) {
    const SCFG::Word &ntSought = valPair.first;
    const Moses2::HypothesisColl *hypos = valPair.second;
    //cerr << "ntSought=" << ntSought << ntSought.isNonTerminal << endl;
    LookupGivenWord(pool, prevPath, ntSought, hypos, subPhraseRange, outPath);
  }
}

void PhraseTableMemory::LookupGivenWord(
    MemPool &pool,
    const SCFG::InputPath &prevPath,
    const SCFG::Word &wordSought,
    const Moses2::HypothesisColl *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const
{
  size_t ptInd = GetPtInd();


  BOOST_FOREACH(const SCFG::ActiveChartEntry *prevEntry, *prevPath.GetActiveChart(ptInd).entries) {
    const ActiveChartEntryMem *prevEntryCast = static_cast<const ActiveChartEntryMem*>(prevEntry);
    //cerr << "entry=" << &entryCast->node << endl;

    //cerr << "BEFORE LookupGivenNode=" << prevPath << endl;
    LookupGivenNode(pool, *prevEntryCast, wordSought, hypos, subPhraseRange, outPath);
    //cerr << "AFTER LookupGivenNode=" << prevPath << endl;
  }
}

void PhraseTableMemory::LookupGivenNode(
    MemPool &pool,
    const ActiveChartEntryMem &prevEntry,
    const SCFG::Word &wordSought,
    const Moses2::HypothesisColl *hypos,
    const Moses2::Range &subPhraseRange,
    SCFG::InputPath &outPath) const
{
  const SCFGNODE &prevNode = prevEntry.node;
  UTIL_THROW_IF2(&prevNode == NULL, "node == NULL");

  size_t ptInd = GetPtInd();
  const SCFGNODE *nextNode = prevNode.Find(wordSought);

  //cerr << "prevEntry=" << *prevEntry.symbolBinds << endl;

  if (nextNode) {
    // new entries
    ActiveChartEntryMem *chartEntry = new (pool.Allocate<ActiveChartEntryMem>()) ActiveChartEntryMem(pool, *nextNode, prevEntry);

    SCFG::SymbolBind &symbolBind = chartEntry->GetSymbolBind();
    symbolBind.Add(subPhraseRange, wordSought, hypos);
    //cerr << "AFTER Add=" << symbolBind << endl;

    outPath.AddActiveChartEntry(ptInd, chartEntry);

    // there are some rules
    AddTargetPhrasesToPath(pool, *nextNode, symbolBind, outPath);

    //cerr << "AFTER outPath=" << outPath << endl;
  }
}

void PhraseTableMemory::AddTargetPhrasesToPath(
    MemPool &pool,
    const SCFGNODE &node,
    const SCFG::SymbolBind &symbolBind,
    SCFG::InputPath &outPath) const
{
  const SCFG::TargetPhrases *tps = node.GetTargetPhrases();
  if (tps) {
    SCFG::TargetPhrases::const_iterator iter;
    for (iter = tps->begin(); iter != tps->end(); ++iter) {
      const SCFG::TargetPhraseImpl *tp = *iter;
      //cerr << "tpCast=" << *tp << endl;
      outPath.AddTargetPhrase(pool, *this, symbolBind, tp);
    }
  }
}

}

