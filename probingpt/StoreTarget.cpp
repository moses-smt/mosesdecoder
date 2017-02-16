/*
 * StoreTarget.cpp
 *
 *  Created on: 19 Jan 2016
 *      Author: hieu
 */
#include <boost/foreach.hpp>
#include "StoreTarget.h"
#include "line_splitter.h"
#include "probing_hash_utils.h"
#include "OutputFileStream.h"
#include "moses2/legacy/Util2.h"

using namespace std;

namespace probingpt
{

StoreTarget::StoreTarget(const std::string &basepath)
  :m_basePath(basepath)
  ,m_vocab(basepath + "/TargetVocab.dat")
{
  std::string path = basepath + "/TargetColl.dat";
  m_fileTargetColl.open(path.c_str(),
                        std::ios::out | std::ios::binary | std::ios::ate | std::ios::trunc);
  if (!m_fileTargetColl.is_open()) {
    throw "can't create file ";
  }

}

StoreTarget::~StoreTarget()
{
  assert(m_coll.empty());
  m_fileTargetColl.close();

  // vocab
  m_vocab.Save();
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
  Moses2::RemoveAllInColl(m_coll);
  m_coll.clear();

  // starting position of coll
  return ret;
}

void StoreTarget::Save(const target_text &rule)
{
  // metadata for each tp
  TargetPhraseInfo tpInfo;
  tpInfo.alignTerm = GetAlignId(rule.word_align_term);
  tpInfo.alignNonTerm = GetAlignId(rule.word_align_non_term);
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

void StoreTarget::SaveAlignment()
{
  std::string path = m_basePath + "/Alignments.dat";
  probingpt::OutputFileStream file(path);

  BOOST_FOREACH(Alignments::value_type &valPair, m_aligns) {
    file << valPair.second << "\t";

    const std::vector<size_t> &aligns = valPair.first;
    BOOST_FOREACH(size_t align, aligns) {
      file << align << " ";
    }
    file << endl;
  }

}

void StoreTarget::Append(const line_text &line, bool log_prob, bool scfg)
{
  target_text *rule = new target_text;
  //cerr << "line.target_phrase=" << line.target_phrase << endl;

  // target_phrase
  vector<bool> nonTerms;
  util::TokenIter<util::SingleCharacter> it;
  it = util::TokenIter<util::SingleCharacter>(line.target_phrase,
       util::SingleCharacter(' '));
  while (it) {
    StringPiece word = *it;
    //cerr << "word=" << word << endl;

    bool nonTerm = false;
    if (scfg) {
      // not really sure how to handle factored SCFG and NT
      if (scfg && word[0] == '[' && word[word.size() - 1] == ']') {
        //cerr << "NON-TERM=" << tok << " " << nonTerms.size() << endl;
        nonTerm = true;
      }
      nonTerms.push_back(nonTerm);
    }

    util::TokenIter<util::SingleCharacter> itFactor;
    itFactor = util::TokenIter<util::SingleCharacter>(word,
               util::SingleCharacter('|'));
    while (itFactor) {
      StringPiece factor = *itFactor;

      string factorStr = factor.as_string();
      uint32_t vocabId = m_vocab.GetVocabId(factorStr);

      rule->target_phrase.push_back(vocabId);

      itFactor++;
    }

    it++;
  }

  // probs
  it = util::TokenIter<util::SingleCharacter>(line.prob,
       util::SingleCharacter(' '));
  while (it) {
    string tok = it->as_string();
    float prob = Moses2::Scan<float>(tok);

    if (log_prob) {
      prob = Moses2::FloorScore(log(prob));
      if (prob == 0.0f) prob = 0.0000000001;
    }

    rule->prob.push_back(prob);
    it++;
  }

  /*
  cerr << "nonTerms=";
  for (size_t i = 0; i < nonTerms.size(); ++i) {
    cerr << nonTerms[i] << " ";
  }
  cerr << endl;
  */

  // alignment
  it = util::TokenIter<util::SingleCharacter>(line.word_align,
       util::SingleCharacter(' '));
  while (it) {
    string tokPair = Moses2::Trim(it->as_string());
    if (tokPair.empty()) {
      break;
    }

    vector<size_t> alignPair = Moses2::Tokenize<size_t>(tokPair, "-");
    assert(alignPair.size() == 2);

    bool nonTerm = false;
    size_t sourcePos = alignPair[0];
    size_t targetPos = alignPair[1];
    if (scfg) {
      nonTerm = nonTerms[targetPos];
    }

    //cerr << targetPos << "=" << nonTerm << endl;

    if (nonTerm) {
      rule->word_align_non_term.push_back(sourcePos);
      rule->word_align_non_term.push_back(targetPos);
      //cerr << (int) rule->word_all1.back() << " ";
    } else {
      rule->word_align_term.push_back(sourcePos);
      rule->word_align_term.push_back(targetPos);
    }

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

uint32_t StoreTarget::GetAlignId(const std::vector<size_t> &align)
{
  boost::unordered_map<std::vector<size_t>, uint32_t>::iterator iter =
    m_aligns.find(align);
  if (iter == m_aligns.end()) {
    uint32_t ind = m_aligns.size();
    m_aligns[align] = ind;
    return ind;
  } else {
    return iter->second;
  }
}

void StoreTarget::AppendLexRO(std::string &prop, std::vector<float> &retvector,
                              bool log_prob) const
{
  size_t startPos = prop.find("{{LexRO ");

  if (startPos != string::npos) {
    size_t endPos = prop.find("}}", startPos + 8);
    string lexProb = prop.substr(startPos + 8, endPos - startPos - 8);
    //cerr << "lexProb=" << lexProb << endl;

    // append lex probs to pt probs
    vector<float> scores = Moses2::Tokenize<float>(lexProb);

    if (log_prob) {
      for (size_t i = 0; i < scores.size(); ++i) {
        scores[i] = Moses2::FloorScore(log(scores[i]));
        if (scores[i] == 0.0f) scores[i] = 0.0000000001;
      }
    }

    for (size_t i = 0; i < scores.size(); ++i) {
      retvector.push_back(scores[i]);
    }

    // exclude LexRO property from property column
    prop = prop.substr(0, startPos)
           + prop.substr(endPos + 2, prop.size() - endPos - 2);
    //cerr << "line.property_to_be_binarized=" << line.property_to_be_binarized << "AAAA" << endl;
  }
}

} /* namespace Moses2 */
