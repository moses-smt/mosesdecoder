/*
 * KBestExtractor.h
 *
 *  Created on: 2 Aug 2016
 *      Author: hieu
 */
#pragma once
#include <vector>
#include <sstream>

namespace Moses2
{
namespace SCFG
{
class Manager;
class TrellisPath;

class KBestExtractor
{
public:
  KBestExtractor(const SCFG::Manager &mgr);
  virtual ~KBestExtractor();

  void OutputToStream(std::stringstream &strm);
protected:
  const SCFG::Manager &m_mgr;

  typedef std::vector<SCFG::TrellisPath*> Coll;
  Coll m_coll;
};

}
} /* namespace Moses2 */
