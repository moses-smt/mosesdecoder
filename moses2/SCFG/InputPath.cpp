/*
 * InputPath.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "InputPath.h"
#include "TargetPhrases.h"
#include "ActiveChart.h"
#include "../TranslationModel/PhraseTable.h"
#include "../MemPoolAllocator.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

InputPath::InputPath(MemPool &pool, const SubPhrase<SCFG::Word> &subPhrase,
                     const Range &range, size_t numPt, const InputPath *prefixPath)
  :InputPathBase(pool, range, numPt, prefixPath)
  ,subPhrase(subPhrase)
  ,targetPhrases(MemPoolAllocator<Element>(pool))
{
  m_activeChart = pool.Allocate<ActiveChart>(numPt);
  for (size_t i = 0; i < numPt; ++i) {
    ActiveChart &memAddr = m_activeChart[i];
    new (&memAddr) ActiveChart(pool);
  }
}

InputPath::~InputPath()
{
  // TODO Auto-generated destructor stub
}

std::string InputPath::Debug(const System &system) const
{
  stringstream out;
  out << range << " ";
  out << subPhrase.Debug(system);
  out << " " << prefixPath << " ";

  const Vector<ActiveChartEntry*> &activeEntries = GetActiveChart(1).entries;
  out << "m_activeChart=" << activeEntries.size() << " ";

  for (size_t i = 0; i < activeEntries.size(); ++i) {
    const ActiveChartEntry &entry = *activeEntries[i];
    out << entry.GetSymbolBind().Debug(system);
    out << "| ";
  }

  // tps
  out << "tps=" << targetPhrases.size();

  out << " ";
  BOOST_FOREACH(const SCFG::InputPath::Coll::value_type &valPair, targetPhrases) {
    const SymbolBind &symbolBind = valPair.first;
    const SCFG::TargetPhrases &tps = *valPair.second;
    out << symbolBind.Debug(system);
    //out << "=" << tps.GetSize() << " ";
    out << tps.Debug(system);
  }

  return out.str();
}

void InputPath::AddTargetPhrasesToPath(
  MemPool &pool,
  const System &system,
  const PhraseTable &pt,
  const SCFG::TargetPhrases &tps,
  const SCFG::SymbolBind &symbolBind)
{
  targetPhrases.push_back(Element(symbolBind, &tps));
  /*
  Coll::iterator iterColl;
  iterColl = targetPhrases.find(symbolBind);
  assert(iterColl == targetPhrases.end());

  targetPhrases[symbolBind] = &tps;
  //cerr << "range=" << range << " symbolBind=" << symbolBind.Debug(system) << " tps=" << tps.Debug(system);
  */
  /*
  SCFG::TargetPhrases *tpsNew;
  tpsNew = new (pool.Allocate<SCFG::TargetPhrases>()) SCFG::TargetPhrases(pool);
  targetPhrases[symbolBind] = tpsNew;

  SCFG::TargetPhrases::const_iterator iter;
  for (iter = tps.begin(); iter != tps.end(); ++iter) {
    const SCFG::TargetPhraseImpl *tp = *iter;
    //cerr << "tpCast=" << *tp << endl;
    tpsNew->AddTargetPhrase(*tp);
  }
  cerr << "range=" << range << " symbolBind=" << symbolBind.Debug(system) << " tpsNew=" << tpsNew->Debug(system);
  */
}

void InputPath::AddActiveChartEntry(size_t ptInd, ActiveChartEntry *chartEntry)
{
  //cerr << "      added " << chartEntry << " " << range << " " << ptInd << endl;
  ActiveChart &activeChart = m_activeChart[ptInd];
  activeChart.entries.push_back(chartEntry);
}

size_t InputPath::GetNumRules() const
{
  size_t ret = 0;
  BOOST_FOREACH(const Coll::value_type &valPair, targetPhrases) {
    const SCFG::TargetPhrases &tps = *valPair.second;
    ret += tps.GetSize();
  }
  return ret;
}

}
}

