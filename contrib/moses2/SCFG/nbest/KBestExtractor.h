/*
 * KBestExtractor.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <sstream>
#include <boost/unordered_map.hpp>
#include "NBest.h"
#include "NBests.h"
#include "NBestColl.h"

namespace Moses2
{
class Scores;

namespace SCFG
{
class Manager;
class Hypothesis;
class NBests;
class NBestScoreOrderer;

/////////////////////////////////////////////////////////////
class KBestExtractor
{
public:
  KBestExtractor(const SCFG::Manager &mgr);
  virtual ~KBestExtractor();

  void OutputToStream(std::stringstream &strm);
protected:
  const SCFG::Manager &m_mgr;
  NBestColl m_nbestColl;
};

}
} /* namespace Moses2 */
