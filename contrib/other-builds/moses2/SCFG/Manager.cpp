/*
 * Manager.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include <cstdlib>
#include <vector>
#include <sstream>
#include "../System.h"
#include "../TranslationModel/PhraseTable.h"
#include "Manager.h"
#include "InputPath.h"
#include "Hypothesis.h"
#include "TargetPhraseImpl.h"
#include "ActiveChart.h"
#include "Sentence.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

Manager::Manager(System &sys, const TranslationTask &task,
    const std::string &inputStr, long translationId) :
    ManagerBase(sys, task, inputStr, translationId)
{

}

Manager::~Manager()
{

}

void Manager::Decode()
{
  // init pools etc
  //cerr << "START InitPools()" << endl;
  InitPools();
  //cerr << "START ParseInput()" << endl;

  FactorCollection &vocab = system.GetVocab();
  m_input = Sentence::CreateFromString(GetPool(), vocab, system, m_inputStr,
      m_translationId);

  const SCFG::Sentence &sentence = static_cast<const SCFG::Sentence&>(GetInput());

  size_t inputSize = sentence.GetSize();
  //cerr << "inputSize=" << inputSize << endl;

  m_inputPaths.Init(sentence, *this);
  //cerr << "CREATED m_inputPaths" << m_inputPaths << endl;

  m_stacks.Init(*this, inputSize);
  //cerr << "CREATED m_stacks" << endl;

  for (int startPos = inputSize - 1; startPos >= 0; --startPos) {
    //cerr << endl << "startPos=" << startPos << endl;
    SCFG::InputPath &path = *m_inputPaths.GetMatrix().GetValue(startPos, 0);

    //cerr << "BEFORE path=" << m_inputPaths << endl;
    InitActiveChart(path);
    //cerr << "AFTER path=" << m_inputPaths << endl;

    int maxPhraseSize = inputSize - startPos + 1;
    for (int phraseSize = 1; phraseSize < maxPhraseSize; ++phraseSize) {
      InputPath &path = *m_inputPaths.GetMatrix().GetValue(startPos, phraseSize);
      //cerr << endl << "phraseSize=" << phraseSize << endl;

      Stack &stack = m_stacks.GetStack(startPos, phraseSize);

      //cerr << "BEFORE LOOKUP path=" << m_inputPaths << endl;
      Lookup(path);
      //cerr << "AFTER LOOKUP path=" << m_inputPaths << endl;

      Decode(path, stack);
      //cerr << "AFTER DECODE path=" << m_inputPaths << endl;

      LookupUnary(path);
      //cerr << "AFTER LookupUnary path=" << m_inputPaths << endl;

      //cerr << "#rules=" << path.GetNumRules() << endl;
    }
  }

  m_stacks.OutputStacks();
}

void Manager::InitActiveChart(SCFG::InputPath &path)
{
   size_t numPt = system.mappings.size();
   //cerr << "numPt=" << numPt << endl;

   for (size_t i = 0; i < numPt; ++i) {
     const PhraseTable &pt = *system.mappings[i];
     //cerr << "START InitActiveChart" << endl;
     pt.InitActiveChart(GetPool(), path);
     //cerr << "FINISHED InitActiveChart" << endl;
   }
}

void Manager::Lookup(SCFG::InputPath &path)
{
  size_t numPt = system.mappings.size();
  //cerr << "numPt=" << numPt << endl;

  for (size_t i = 0; i < numPt; ++i) {
    const PhraseTable &pt = *system.mappings[i];
    size_t maxChartSpan = system.maxChartSpans[i];
    pt.Lookup(GetPool(), *this, maxChartSpan, m_stacks, path);
  }

  /*
  size_t tpsNum = path.targetPhrases.GetSize();
  if (tpsNum) {
    cerr << tpsNum << " " << path << endl;
  }
  */
}

void Manager::LookupUnary(SCFG::InputPath &path)
{
  size_t numPt = system.mappings.size();
  //cerr << "numPt=" << numPt << endl;

  for (size_t i = 0; i < numPt; ++i) {
    const PhraseTable &pt = *system.mappings[i];
    pt.LookupUnary(GetPool(), *this, m_stacks, path);
  }

  /*
  size_t tpsNum = path.targetPhrases.GetSize();
  if (tpsNum) {
    cerr << tpsNum << " " << path << endl;
  }
  */
}

///////////////////////////////////////////////////////////////
// CUBE-PRUNING
///////////////////////////////////////////////////////////////
void Manager::Decode(SCFG::InputPath &path, Stack &stack)
{
  BOOST_FOREACH(const InputPath::Coll::value_type &valPair, *path.targetPhrases) {
    const SymbolBind &symbolBind = valPair.first;
    const SCFG::TargetPhrases &tps = *valPair.second;
    CreateQueue(path, symbolBind, tps);
  }

  size_t pops = 0;
  while (!m_queue.empty() && pops < system.options.cube.pop_limit) {
    QueueItem *item = m_queue.top();
    m_queue.pop();

    // add hypo to stack
    Hypothesis *hypo = item->hypo;

    //cerr << "hypo=" << *hypo << " " << hypo->GetBitmap() << endl;
    stack.Add(hypo, GetHypoRecycle(), arcLists);

    item->CreateNext(*this, m_queue, path);

    ++pops;
  }

}

void Manager::CreateQueue(
    const SCFG::InputPath &path,
    const SymbolBind &symbolBind,
    const SCFG::TargetPhrases &tps)
{
  QueueItem *item = new QueueItem(tps);
  for (size_t i = 0; i < symbolBind.coll.size(); ++i) {
    const SymbolBindElement &ele = symbolBind.coll[i];
    if (ele.hypos) {
      item->AddHypos(*ele.hypos);
    }
  }
  item->CreateHypo(*this, path, symbolBind);
  m_queue.push(item);
}

///////////////////////////////////////////////////////////////
// NON CUBE-PRUNING
///////////////////////////////////////////////////////////////
/*
void Manager::Decode(SCFG::InputPath &path, Stack &stack)
{
  //cerr << "path=" << path << endl;

  boost::unordered_map<SCFG::SymbolBind, SCFG::TargetPhrases*>::const_iterator iterOuter;
  for (iterOuter = path.targetPhrases->begin(); iterOuter != path.targetPhrases->end(); ++iterOuter) {
    const SCFG::SymbolBind &symbolBind = iterOuter->first;

    const SCFG::TargetPhrases &tps = *iterOuter->second;
    //cerr << "symbolBind=" << symbolBind << " tps=" << tps.GetSize() << endl;

    SCFG::TargetPhrases::const_iterator iter;
    for (iter = tps.begin(); iter != tps.end(); ++iter) {
      const SCFG::TargetPhraseImpl &tp = **iter;
      //cerr << "tp=" << tp << endl;
      ExpandHypo(path, symbolBind, tp, stack);
    }
  }
}
*/

void Manager::ExpandHypo(
    const SCFG::InputPath &path,
    const SCFG::SymbolBind &symbolBind,
    const SCFG::TargetPhraseImpl &tp,
    Stack &stack)
{
  Recycler<HypothesisBase*> &hypoRecycler = GetHypoRecycle();

  std::vector<const SymbolBindElement*> ntEles = symbolBind.GetNTElements();
  vector<size_t> prevHyposIndices(symbolBind.numNT);
  assert(ntEles.size() == symbolBind.numNT);
  //cerr << "ntEles:" << ntEles.size() << endl;

  size_t ind = 0;
  while (IncrPrevHypoIndices(prevHyposIndices, ind, ntEles)) {
    SCFG::Hypothesis *hypo = SCFG::Hypothesis::Create(GetPool(), *this);
    hypo->Init(*this, path, symbolBind, tp, prevHyposIndices);
    hypo->EvaluateWhenApplied();

    stack.Add(hypo, hypoRecycler, arcLists);

    ++ind;
  }
}

bool Manager::IncrPrevHypoIndices(
    vector<size_t> &prevHyposIndices,
    size_t ind,
    const std::vector<const SymbolBindElement*> ntEles)
{
  if (ntEles.size() == 0) {
    // no nt. Do the 1st
    return ind ? false : true;
  }

  size_t numHypos = 0;

  //cerr << "IncrPrevHypoIndices:" << ind << " " << ntEles.size() << " ";
  for (size_t i = 0; i < ntEles.size() - 1; ++i) {
    const SymbolBindElement &ele = *ntEles[i];
    Hypotheses &hypos = ele.hypos->GetSortedAndPruneHypos(*this, arcLists);
    numHypos = hypos.size();

    std::div_t divRet = std::div((int)ind, (int)numHypos);
    ind = divRet.quot;

    size_t hypoInd = divRet.rem;
    prevHyposIndices[i] = hypoInd;
    //cerr << "(" << i << "," << ind << "," << numHypos << "," << hypoInd << ")";
  }

  // last
  prevHyposIndices.back() = ind;


  // check if last is over limit
  const SymbolBindElement &ele = *ntEles.back();
  Hypotheses &hypos = ele.hypos->GetSortedAndPruneHypos(*this, arcLists);
  numHypos = hypos.size();

  //cerr << "(" << (ntEles.size() - 1) << "," << ind << "," << numHypos << ","  << ind << ")";
  //cerr << endl;

  if (ind >= numHypos) {
    return false;
  }
  else {
    return true;
  }
}

std::string Manager::OutputBest() const
{
  stringstream out;

  const Stack &lastStack = m_stacks.GetLastStack();
  const Hypothesis *bestHypo = lastStack.GetBestHypo(*this, const_cast<ArcLists&>(arcLists));

  if (bestHypo) {
    bestHypo->OutputToStream(out);
    cerr << "BEST TRANSLATION: " << *bestHypo << " " << out.str() << endl;
  }
  else {
    cerr << "NO TRANSLATION " << m_input->GetTranslationId() << endl;
  }

  out << endl;
  return out.str();
}

} // namespace
}

