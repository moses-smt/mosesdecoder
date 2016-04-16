/*
 * InputPath.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include <vector>
#include "../InputPathBase.h"
#include "TargetPhrases.h"

namespace Moses2
{
namespace SCFG
{
class TargetPhrases;
class TargetPhraseImpl;

class ActiveChartEntry
{
public:
  ActiveChartEntry()
  { }
};

////////////////////////////////////////////////////////////////////////////
class ActiveChart
{
public:
  std::vector<ActiveChartEntry*> entries;
};

////////////////////////////////////////////////////////////////////////////
class InputPath: public InputPathBase
{
  friend std::ostream& operator<<(std::ostream &, const InputPath &);
public:
  SCFG::TargetPhrases targetPhrases;

  InputPath(MemPool &pool, const SubPhrase &subPhrase, const Range &range,
      size_t numPt, const InputPath *prefixPath);
  virtual ~InputPath();

  const ActiveChart &GetActiveChart(size_t ptInd) const
  { return m_activeChart[ptInd]; }
  ActiveChart &GetActiveChart(size_t ptInd)
  { return m_activeChart[ptInd]; }

  void AddTargetPhrase(const PhraseTable &pt, const SCFG::TargetPhraseImpl *tp);

protected:
  ActiveChart *m_activeChart;
};

}
}

