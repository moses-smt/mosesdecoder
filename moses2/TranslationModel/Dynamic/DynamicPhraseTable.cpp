/*
 * DynamicPhraseTable.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <sstream>
#include <boost/foreach.hpp>
#include "DynamicPhraseTable.h"
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
#include "../../PhraseBased/SentenceWithCandidates.h"

#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"
#include "../../SCFG/InputPath.h"
#include "../../SCFG/Stack.h"
#include "../../SCFG/Stacks.h"
#include "../../SCFG/Manager.h"

#include "../../PhraseBased/SentenceWithCandidates.h"
#include "../../PhraseBased/Manager.h"

using namespace std;

namespace Moses2
{
thread_local DynamicPhraseTable::PBNODE DynamicPhraseTable::m_rootPb;

////////////////////////////////////////////////////////////////////////

DynamicPhraseTable::DynamicPhraseTable(size_t startInd, const std::string &line)
  :PhraseTable(startInd, line)
{
  ReadParameters();
}

DynamicPhraseTable::~DynamicPhraseTable()
{
  m_rootPb.CleanNode();
}

void DynamicPhraseTable::CreatePTForInput(const ManagerBase &mgr, string phraseTableString)
{
  //cerr << "In CreatePTForInput" << endl << flush;
  const System &system = mgr.system;
  FactorCollection &vocab = system.GetVocab();
  MemPool &pool = mgr.GetPool();
  MemPool tmpSourcePool;

  if (system.isPb) {
    //m_rootPb = new PBNODE();
  } else {
    abort();
    //cerr << "m_rootSCFG=" << m_rootSCFG << endl;
  }

  vector<string> toks;
  size_t lineNum = 0;
  istringstream  strme(phraseTableString);
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
      TargetPhraseImpl *target = TargetPhraseImpl::CreateFromString(pool, *this, system,
                                 toks[1]);
      //cerr << "created target" << endl;
      target->GetScores().CreateFromString(toks[2], *this, system, true);
      //cerr << "created scores:" << *target << endl;

      if (toks.size() >= 4) {
        //cerr << "alignstr=" << toks[3] << endl;
        target->SetAlignmentInfo(toks[3]);
      }

      // properties
      if (toks.size() == 7) {
        //target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
        //strcpy(target->properties, toks[6].c_str());
      }

      system.featureFunctions.EvaluateInIsolation(pool, system, *source,
          *target);
      //cerr << "EvaluateInIsolation:" << target->Debug(system) << endl;
      m_rootPb.AddRule(m_input, *source, target);

      //cerr << "target=" << target->Debug(system) << endl;
    } else {
      abort();
    }
  }

  if (system.isPb) {
    m_rootPb.SortAndPrune(m_tableLimit, pool, system);
    //cerr << "root=" << &m_rootPb << endl;
  } else {
      abort();
  }
  /*
  BOOST_FOREACH(const PtMem::Node<Word>::Children::value_type &valPair, m_rootPb.GetChildren()) {
    const Word &word = valPair.first;
    cerr << word << " ";
  }
  cerr << endl;
  */

}

void DynamicPhraseTable::InitializeForInput(const ManagerBase &mgr, const InputType &input)
{
  // downcast to SentenceWithCandidates
  const SentenceWithCandidates &inputObj = static_cast<const SentenceWithCandidates&>(input);
  CreatePTForInput(mgr, inputObj.getPhraseTableString());
}

TargetPhrases* DynamicPhraseTable::Lookup(const Manager &mgr, MemPool &pool,
    InputPath &inputPath) const
{
  const SubPhrase<Moses2::Word> &phrase = inputPath.subPhrase;
  TargetPhrases *tps = m_rootPb.Find(m_input, phrase);
  return tps;
}

void DynamicPhraseTable::CleanUpAfterSentenceProcessing(const System &system, const InputType &input) const {
   m_rootPb.CleanNode(); //TODO  : clean this
}

void DynamicPhraseTable::InitActiveChart(
  MemPool &pool,
  const SCFG::Manager &mgr,
  SCFG::InputPath &path) const
{
  abort();
}

void DynamicPhraseTable::Lookup(MemPool &pool,
                               const SCFG::Manager &mgr,
                               size_t maxChartSpan,
                               const SCFG::Stacks &stacks,
                               SCFG::InputPath &path) const
{
  abort();
}

void DynamicPhraseTable::LookupGivenNode(
  MemPool &pool,
  const SCFG::Manager &mgr,
  const SCFG::ActiveChartEntry &prevEntry,
  const SCFG::Word &wordSought,
  const Moses2::Hypotheses *hypos,
  const Moses2::Range &subPhraseRange,
  SCFG::InputPath &outPath) const
{
  abort();
}

}

