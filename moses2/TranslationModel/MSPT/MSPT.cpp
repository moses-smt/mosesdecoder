/*
 * MSPT.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <boost/foreach.hpp>
#include "MSPT.h"
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

MSPT::MSPT(size_t startInd, const std::string &line)
  :PhraseTable(startInd, line)
  ,m_rootPb(NULL)
  ,m_rootSCFG(NULL)
{
  ReadParameters();
}

MSPT::~MSPT()
{
  delete m_rootPb;
  delete m_rootSCFG;
}

void MSPT::InitializeForInput(const InputType &input)
{
  cerr << "InitializeForInput" << endl;
}

TargetPhrases* MSPT::Lookup(const Manager &mgr, MemPool &pool,
    InputPath &inputPath) const
{
  const SubPhrase<Moses2::Word> &phrase = inputPath.subPhrase;
  TargetPhrases *tps = m_rootPb->Find(m_input, phrase);
  return tps;
}

void MSPT::InitActiveChart(
  MemPool &pool,
  const SCFG::Manager &mgr,
  SCFG::InputPath &path) const
{
  size_t ptInd = GetPtInd();
  ActiveChartEntryMem *chartEntry = new (pool.Allocate<ActiveChartEntryMem>()) ActiveChartEntryMem(pool, *m_rootSCFG);
  path.AddActiveChartEntry(ptInd, chartEntry);
  //cerr << "InitActiveChart=" << path << endl;
}

void MSPT::Lookup(MemPool &pool,
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
  LookupGivenWord(pool, mgr, *prevPath, lastWord, NULL, subPhrasePath.range, path);
  //cerr << "AFTER LookupGivenWord=" << *prevPath << endl;

  // NON-TERMINAL
  //const SCFG::InputPath *prefixPath = static_cast<const SCFG::InputPath*>(path.prefixPath);
  while (prevPath) {
    const Range &prevRange = prevPath->range;
    //cerr << "prevRange=" << prevRange << endl;

    size_t startPos = prevRange.GetEndPos() + 1;
    size_t ntSize = endPos - startPos + 1;
    const SCFG::InputPath &subPhrasePath = *mgr.GetInputPaths().GetMatrix().GetValue(startPos, ntSize);

    LookupNT(pool, mgr, subPhrasePath.range, *prevPath, stacks, path);

    prevPath = static_cast<const SCFG::InputPath*>(prevPath->prefixPath);
  }
}

void MSPT::LookupGivenNode(
  MemPool &pool,
  const SCFG::Manager &mgr,
  const SCFG::ActiveChartEntry &prevEntry,
  const SCFG::Word &wordSought,
  const Moses2::Hypotheses *hypos,
  const Moses2::Range &subPhraseRange,
  SCFG::InputPath &outPath) const
{
  const ActiveChartEntryMem &prevEntryCast = static_cast<const ActiveChartEntryMem&>(prevEntry);

  const SCFGNODE &prevNode = prevEntryCast.node;
  UTIL_THROW_IF2(&prevNode == NULL, "node == NULL");

  size_t ptInd = GetPtInd();
  const SCFGNODE *nextNode = prevNode.Find(m_input, wordSought);

  /*
  if (outPath.range.GetStartPos() == 1 || outPath.range.GetStartPos() == 2) {
    cerr  << "range=" << outPath.range
          << " prevEntry=" << prevEntry.GetSymbolBind().Debug(mgr.system)
          << " wordSought=" << wordSought.Debug(mgr.system)
          << " nextNode=" << nextNode
          << endl;
  }
  */
  if (nextNode) {
    // new entries
    ActiveChartEntryMem *chartEntry = new (pool.Allocate<ActiveChartEntryMem>()) ActiveChartEntryMem(pool, *nextNode, prevEntry);

    chartEntry->AddSymbolBindElement(subPhraseRange, wordSought, hypos, *this);
    //cerr << "AFTER Add=" << symbolBind << endl;

    outPath.AddActiveChartEntry(ptInd, chartEntry);

    const SCFG::TargetPhrases *tps = nextNode->GetTargetPhrases();
    if (tps) {
      // there are some rules
      /*
      cerr << "outPath=" << outPath.range
      	  << " bind=" << chartEntry->GetSymbolBind().Debug(mgr.system)
      	  << " pt=" << GetPtInd()
        << " tps=" << tps->Debug(mgr.system) << endl;
      */
      outPath.AddTargetPhrasesToPath(pool, mgr.system, *this, *tps, chartEntry->GetSymbolBind());

    }

    //cerr << "AFTER outPath=" << outPath << endl;
  }
}

}

