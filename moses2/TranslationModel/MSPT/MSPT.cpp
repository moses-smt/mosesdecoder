/*
 * MSPT.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <sstream>
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
#include "../../PhraseBased/SentenceWithCandidates.h"

#include "../../SCFG/PhraseImpl.h"
#include "../../SCFG/TargetPhraseImpl.h"
#include "../../SCFG/InputPath.h"
#include "../../SCFG/Stack.h"
#include "../../SCFG/Stacks.h"
#include "../../SCFG/Manager.h"

#include "../../PhraseBased/SentenceWithCandidates.h"

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

// void MSPT::CreatePTForInput(string phraseTableString)
// {
//   FactorCollection &vocab = system.GetVocab();
//   MemPool &systemPool = system.GetSystemPool();
//   MemPool tmpSourcePool;

//   if (system.isPb) {
//     m_rootPb = new PBNODE();
//   } else {
//     m_rootSCFG = new SCFGNODE();
//     //cerr << "m_rootSCFG=" << m_rootSCFG << endl;
//   }

//   vector<string> toks;
//   size_t lineNum = 0;
//   istringstream  strme(phraseTableString);
//   string line;
//   while (getline(strme, line)) {
//     if (++lineNum % 1000000 == 0) {
//       cerr << lineNum << " ";
//     }
//     toks.clear();
//     TokenizeMultiCharSeparator(toks, line, "|||");
//     UTIL_THROW_IF2(toks.size() < 3, "Wrong format");
//     //cerr << "line=" << line << endl;
//     //cerr << "system.isPb=" << system.isPb << endl;

//     if (system.isPb) {
//       PhraseImpl *source = PhraseImpl::CreateFromString(tmpSourcePool, vocab, system,
//                            toks[0]);
//       //cerr << "created soure" << endl;
//       TargetPhraseImpl *target = TargetPhraseImpl::CreateFromString(systemPool, *this, system,
//                                  toks[1]);
//       //cerr << "created target" << endl;
//       target->GetScores().CreateFromString(toks[2], *this, system, true);
//       //cerr << "created scores:" << *target << endl;

//       if (toks.size() >= 4) {
//         //cerr << "alignstr=" << toks[3] << endl;
//         target->SetAlignmentInfo(toks[3]);
//       }

//       // properties
//       if (toks.size() == 7) {
//         //target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
//         //strcpy(target->properties, toks[6].c_str());
//       }

//       system.featureFunctions.EvaluateInIsolation(systemPool, system, *source,
//           *target);
//       //cerr << "EvaluateInIsolation:" << *target << endl;
//       m_rootPb->AddRule(m_input, *source, target);

//       //cerr << "target=" << target->Debug(system) << endl;
//     } else {
//       SCFG::PhraseImpl *source = SCFG::PhraseImpl::CreateFromString(tmpSourcePool, vocab, system,
//                                  toks[0]);
//       //cerr << "created source:" << *source << endl;
//       SCFG::TargetPhraseImpl *target = SCFG::TargetPhraseImpl::CreateFromString(systemPool, *this,
//                                        system, toks[1]);

//       //cerr << "created target " << *target << " source=" << *source << endl;

//       target->GetScores().CreateFromString(toks[2], *this, system, true);
//       //cerr << "created scores:" << *target << endl;

//       //vector<SCORE> scores = Tokenize<SCORE>(toks[2]);
//       //target->sortScore = (scores.size() >= 3) ? TransformScore(scores[2]) : 0;

//       target->SetAlignmentInfo(toks[3]);

//       // properties
//       if (toks.size() == 7) {
//         //target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
//         //strcpy(target->properties, toks[6].c_str());
//       }

//       system.featureFunctions.EvaluateInIsolation(systemPool, system, *source,
//           *target);
//       //cerr << "EvaluateInIsolation:" << *target << endl;
//       m_rootSCFG->AddRule(m_input, *source, target);
//     }
//   }

//   if (system.isPb) {
//     m_rootPb->SortAndPrune(m_tableLimit, systemPool, system);
//     //cerr << "root=" << &m_rootPb << endl;
//   } else {
//     m_rootSCFG->SortAndPrune(m_tableLimit, systemPool, system);
//     //cerr << "root=" << &m_rootPb << endl;
//   }
//   /*
//   BOOST_FOREACH(const PtMem::Node<Word>::Children::value_type &valPair, m_rootPb.GetChildren()) {
//     const Word &word = valPair.first;
//     cerr << word << " ";
//   }
//   cerr << endl;
//   */

// }

void MSPT::InitializeForInput(const System &system, const InputType &input)
{
  cerr << "InitializeForInput MSPT" << endl;
  cerr << input.Debug(system) << endl << flush;
  cerr << "HH1" << endl << flush;
  
  // downcast to SentenceWithCandidates
  //const SentenceWithCandidates &inputObj = static_cast<const SentenceWithCandidates&>(input);
  const SentenceWithCandidates &inputObj = dynamic_cast<const SentenceWithCandidates&>(input);
  cerr << "Casting done." << endl << flush;
  cerr << "PhraseTableString member: " << inputObj.getPhraseTableString() << endl;

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
  abort();
}

void MSPT::Lookup(MemPool &pool,
                               const SCFG::Manager &mgr,
                               size_t maxChartSpan,
                               const SCFG::Stacks &stacks,
                               SCFG::InputPath &path) const
{
  abort();
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
  abort();
}

}

