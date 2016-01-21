/*
 * StoreTarget.cpp
 *
 *  Created on: 19 Jan 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "StoreTarget.h"
#include "line_splitter.hh"
#include "probing_hash_utils.hh"
#include "../OutputFileStream.h"
#include "../Util2.h"

using namespace std;

namespace Moses2 {

StoreTarget::StoreTarget(const std::string &basepath)
:m_basePath(basepath)
{
  std::string path = basepath + "/TargetColl.dat";
  m_fileTargetColl.open(path.c_str(), std::ios::out | std::ios::binary | std::ios::ate | std::ios::trunc);
  if (!m_fileTargetColl.is_open()) {
	  throw "can't create file ";
  }

}

StoreTarget::~StoreTarget() {
	assert(m_coll.empty());
	m_fileTargetColl.close();

	// vocab
    std::string path = m_basePath + "/TargetVocab.dat";
    OutputFileStream strme(path);

	boost::unordered_map<std::string, uint32_t>::const_iterator iter;
	for (iter = m_vocab.begin(); iter != m_vocab.end(); ++iter) {
	  strme << iter->first << "\t" << iter->second << endl;
    }

    strme.Close();
}

uint64_t StoreTarget::Save()
{
  uint64_t ret = m_fileTargetColl.tellp();

  // save to disk
  uint64_t numTP = m_coll.size();
  m_fileTargetColl.write((char*) &numTP, sizeof(uint64_t));

  for (size_t i = 0; i < m_coll.size(); ++i) {
	  Save(*m_coll[i]);
  }

  // clear coll
  RemoveAllInColl(m_coll);
  m_coll.clear();

  // starting position of coll
  return ret;
}

void StoreTarget::Save(const target_text &rule)
{
	// metadata for each tp
	TargetPhraseInfo tpInfo;
	tpInfo.alignInd = GetAlignId(rule.word_all1);
	tpInfo.numWords = rule.target_phrase.size();
	tpInfo.propLength = rule.property.size();

	//cerr << "TPInfo=" << sizeof(TPInfo);
    m_fileTargetColl.write((char*) &tpInfo, sizeof(TargetPhraseInfo));

    // scores
    for (size_t i = 0; i < rule.prob.size(); ++i) {
    	float prob = rule.prob[i];
        m_fileTargetColl.write((char*) &prob, sizeof(prob));
    }

    // tp
    for (size_t i = 0; i < rule.target_phrase.size(); ++i) {
    	uint32_t vocabId = rule.target_phrase[i];
        m_fileTargetColl.write((char*) &vocabId, sizeof(vocabId));
    }

    // prop TODO

}

void StoreTarget::Append(const line_text &line, bool log_prob)
{
  target_text *rule = new target_text;

  // target_phrase
  util::TokenIter<util::SingleCharacter> it;
  it = util::TokenIter<util::SingleCharacter>(line.target_phrase, util::SingleCharacter(' '));
  while (it) {
	string tok = it->as_string();
	uint32_t vocabId = GetVocabId(tok);

	rule->target_phrase.push_back(vocabId);
	it++;
  }

  // probs
  it = util::TokenIter<util::SingleCharacter>(line.prob, util::SingleCharacter(' '));
  while (it) {
	string tok = it->as_string();
	float prob = Scan<float>(tok);

	if (log_prob) {
		prob = FloorScore(log(prob));
    	if (prob == 0.0f) prob = 0.0000000001;
    }

	rule->prob.push_back(prob);
	it++;
  }

  // alignment
  it = util::TokenIter<util::SingleCharacter>(line.word_align, util::SingleCharacter(' '));
  while (it) {
	string tokPair = Trim(it->as_string());
	if (tokPair.empty()) {
		break;
	}

	vector<unsigned char> alignPair = Tokenize<unsigned char>(tokPair, "-");
	assert(alignPair.size() == 2);
	rule->word_all1.push_back(alignPair[0]);
	rule->word_all1.push_back(alignPair[1]);
	it++;
  }

   // extra scores
  string prop = line.property.as_string();
  AppendLexRO(prop, rule->prob, log_prob);

  //cerr << "line.property=" << line.property << endl;
  //cerr << "prop=" << prop << endl;

  // properties
  /*
  for (size_t i = 0; i < prop.size(); ++i) {
	  rule->property.push_back(prop[i]);
  }
  */
  m_coll.push_back(rule);
}

uint32_t StoreTarget::GetVocabId(const std::string &word)
{
  boost::unordered_map<std::string, uint32_t>::iterator iter = m_vocab.find(word);
  if (iter == m_vocab.end()) {
	  uint32_t ind = m_vocab.size() + 1;
	  m_vocab[word] = ind;
	  return ind;
  }
  else {
	  return iter->second;
  }
}

uint16_t StoreTarget::GetAlignId(const std::vector<unsigned char> &align)
{
	  boost::unordered_map<std::vector<unsigned char>, uint16_t>::iterator iter = m_aligns.find(align);
	  if (iter == m_aligns.end()) {
		  uint32_t ind = m_aligns.size() + 1;
		  m_aligns[align] = ind;
		  return ind;
	  }
	  else {
		  return iter->second;
	  }
}

void StoreTarget::AppendLexRO(std::string &prop, std::vector<float> &retvector, bool log_prob) const
{
  size_t startPos = prop.find("{{LexRO ");

  if (startPos != string::npos) {
	  size_t endPos = prop.find("}}", startPos + 8);
	  string lexProb = prop.substr(startPos + 8, endPos - startPos - 8);
	  //cerr << "lexProb=" << lexProb << endl;

	  // append lex probs to pt probs
	  vector<float> scores = Tokenize<float>(lexProb);

      if (log_prob) {
    	  for (size_t i = 0; i < scores.size(); ++i) {
    		  scores[i] = FloorScore(log(scores[i]));
    		  if (scores[i] == 0.0f) scores[i] = 0.0000000001;
    	  }
      }

	  for (size_t i = 0; i < scores.size(); ++i) {
		retvector.push_back(scores[i]);
	  }

	  // exclude LexRO property from property column
	  prop = prop.substr(0, startPos)	+ prop.substr(endPos + 2, prop.size() - endPos - 2);
	  //cerr << "line.property_to_be_binarized=" << line.property_to_be_binarized << "AAAA" << endl;
  }
}

} /* namespace Moses2 */
