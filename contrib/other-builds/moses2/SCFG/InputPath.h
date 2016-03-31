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

namespace Moses2
{
namespace SCFG
{
class ActiveChartEntry
{
public:
  const void *data;

  ActiveChartEntry(const void *vdata) :
      data(vdata)
  {
  }
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
  InputPath(MemPool &pool, const SubPhrase &subPhrase, const Range &range,
      size_t numPt, const InputPath *prefixPath);
  virtual ~InputPath();

  ActiveChart &GetActiveChart(size_t ptInd)
  {
    return m_activeChart[ptInd];
  }

protected:
  ActiveChart *m_activeChart;
};

}
}

