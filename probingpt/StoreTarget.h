/*
 * StoreTarget.h
 *
 *  Created on: 19 Jan 2016
 *      Author: hieu
 */
#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <inttypes.h>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "StoreVocab.h"

namespace probingpt
{

class line_text;
class target_text;

class StoreTarget
{
public:
  StoreTarget(const std::string &basepath);
  virtual ~StoreTarget();

  uint64_t Save();
  void SaveAlignment();

  void Append(const line_text &line, bool log_prob, bool scfg);
protected:
  std::string m_basePath;
  std::fstream m_fileTargetColl;
  StoreVocab<uint32_t> m_vocab;

  typedef boost::unordered_map<std::vector<size_t>, uint32_t> Alignments;
  Alignments m_aligns;

  std::vector<target_text*> m_coll;

  uint32_t GetAlignId(const std::vector<size_t> &align);
  void Save(const target_text &rule);

  void AppendLexRO(std::string &prop, std::vector<float> &retvector,
                   bool log_prob) const;

};

} /* namespace Moses2 */

