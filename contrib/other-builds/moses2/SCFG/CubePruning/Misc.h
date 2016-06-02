/*
 * Misc.h
 *
 *  Created on: 2 Jun 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include "../../HypothesisColl.h"

namespace Moses2
{

namespace SCFG
{
class TargetPhrases;

class QueueItem
{
public:
  const SCFG::TargetPhrases &tps;
  size_t tpsInd;

  typedef std::pair<const Moses2::HypothesisColl *, size_t> HyposElement;
  std::vector<HyposElement> hyposColl;
    // hypos and ind to the 1 we're using

  QueueItem(const SCFG::TargetPhrases &tps);
  void AddHypos(const Moses2::HypothesisColl &hypos);

};

class Queue
{
public:
  bool empty() const
  { return true; }
};

}
}



