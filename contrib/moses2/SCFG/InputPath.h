/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include <boost/unordered_map.hpp>
#include "../InputPathBase.h"
#include "TargetPhrases.h"
#include "ActiveChart.h"
#include "Word.h"

namespace Moses2
{
namespace SCFG
{
class TargetPhrases;
class TargetPhraseImpl;


////////////////////////////////////////////////////////////////////////////
class InputPath: public InputPathBase
{
public:
  typedef std::pair<const PhraseTable*, SymbolBind> CollKey;
  typedef boost::unordered_map<SymbolBind,
      SCFG::TargetPhrases*,
      boost::hash<SymbolBind>,
      std::equal_to<SymbolBind>,
      MemPoolAllocator< std::pair<SymbolBind, SCFG::TargetPhrases*> >
      > Coll;
  Coll *targetPhrases;
  SubPhrase<SCFG::Word> subPhrase;

  InputPath(MemPool &pool, const SubPhrase<SCFG::Word> &subPhrase, const Range &range,
      size_t numPt, const InputPath *prefixPath);
  virtual ~InputPath();

  const ActiveChart &GetActiveChart(size_t ptInd) const
  { return m_activeChart[ptInd]; }

  void AddActiveChartEntry(size_t ptInd, ActiveChartEntry *chartEntry);

  void AddTargetPhrasesToPath(
      MemPool &pool,
      const PhraseTable &pt,
      const SCFG::TargetPhrases &tps,
      const SCFG::SymbolBind &symbolBind);

  size_t GetNumRules() const;

  std::string Debug(const System &system) const;

protected:
  ActiveChart *m_activeChart;
};

}
}

