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

namespace Moses2 {

class line_text;
class target_text;

class StoreTarget {
public:
	StoreTarget(const std::string &basepath);
	virtual ~StoreTarget();

	uint64_t Save();
	void Append(const line_text &line, bool log_prob);
protected:
  std::string m_basePath;
  std::fstream m_fileTargetColl;
  boost::unordered_map<std::string, uint32_t> m_vocab;
  boost::unordered_map<std::vector<unsigned char>, uint16_t> m_aligns;

  std::vector<target_text*> m_coll;

  uint32_t GetVocabId(const std::string &word);
  uint16_t GetAlignId(const std::vector<unsigned char> &align);
  void Save(const target_text &rule);

  void AppendLexRO(std::string &prop, std::vector<float> &retvector, bool log_prob) const;

};

} /* namespace Moses2 */

